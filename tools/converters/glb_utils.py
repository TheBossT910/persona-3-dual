import os
import struct
import tempfile
import subprocess

import numpy as np
from PIL import Image

MAX_16_BIT_INT = 32767
MAGIC = b"MDL3"
GRIT_PATH = "/opt/wonderful/thirdparty/blocksds/core/tools/grit/grit"

# NDS GPU Commands
FIFO_BEGIN = 0x40
FIFO_COLOR = 0x20
FIFO_TEXCOORD = 0x22
FIFO_VERTEX16 = 0x23
FIFO_END = 0x41
FIFO_NOP = 0x00
GL_TRIANGLES = 0

# Component type mappings from GLTF to numpy dtypes
DTYPE_MAP = {
    5120: np.int8,
    5121: np.uint8,
    5122: np.int16,
    5123: np.uint16,
    5125: np.uint32,
    5126: np.float32,
}
TYPE_COUNTS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}

# Math Helpers / Type Conversions


def next_pow2(x: int) -> int:
    """Returns the next power of two greater than or equal to x, with a minimum of 8.
    Args:
        x (int): The input integer.
    Returns:
        int: The next power of two greater than or equal to x, or 8 if x <= 8."""
    return 1 << (x - 1).bit_length() if x > 8 else 8


def float_to_v16(val):
    """Converts a float to a Nintendo DS 4.12 fixed-point integer.
    Args:
        val (float): The input float value.
    Returns:
        int: The corresponding 4.12 fixed-point integer, clamped to the range [-32767, 32767].
    """
    i = int(round(val * 4096.0))
    return max(-MAX_16_BIT_INT, min(MAX_16_BIT_INT, i))


def float_to_s16(val):
    """Converts a float to a signed 16-bit integer.
    Args:
        val (float): The input float value.
    Returns:
        int: The corresponding signed 16-bit integer, clamped to the range [-32767, 32767].
    """
    val = int(val) & 0xFFFF
    return val if val <= MAX_16_BIT_INT else val - (2 * MAX_16_BIT_INT)


def quat_float_to_s16(val):
    """Normalizes float [-1.0, 1.0] to signed 16-bit integer.
    Args:
        val (float): The input float value.
    Returns:
        int: The normalized signed 16-bit integer, clamped to the range [-4096, 4096].
    """
    return max(-4096, min(4096, int(round(val * 4096.0))))


def rgb_to_rgb15(color):
    """Converts RGB float (0.0 - 1.0) or int (0 - 255) array to NDS 15-bit RGB integer.
    Args:
        color (list): The input color as a list or array of three values (R, G, B).
    Returns:
        int: The resulting 15-bit RGB integer."""
    if color is None:
        return 0x7FFF  # Default White

    r, g, b = color[0], color[1], color[2]

    # Handle uint8/uint16 byte colors vs float colors
    if isinstance(r, (np.integer, int)) and max(r, g, b) > 1:
        r_5 = (int(r) >> 3) & 0x1F
        g_5 = (int(g) >> 3) & 0x1F
        b_5 = (int(b) >> 3) & 0x1F
    else:
        r_5 = int(max(0.0, min(1.0, float(r))) * 31.0)
        g_5 = int(max(0.0, min(1.0, float(g))) * 31.0)
        b_5 = int(max(0.0, min(1.0, float(b))) * 31.0)

    return r_5 | (g_5 << 5) | (b_5 << 10)


# NDS Packing Functions


def pack_uv_t16(u, v, tex_w, tex_h):
    """Packs UV into DS TEXTURE_PACK t16 format.

    glTF UVs are passed through directly: TEXCOORD values are stored as
        (u * tex_w) and (v * tex_h) then multiplied by 16 (4 fractional bits).

    Args:
        u (float): The U coordinate (0.0 to 1.0).
        v (float): The V coordinate (0.0 to 1.0).
        tex_w (int): The width of the texture in pixels.
        tex_h (int): The height of the texture in pixels.
    Returns:
        int: The packed UV value in t16 format (32-bit integer).
    """
    # scale into texel space and 4 fractional bits (equivalent to <<4)
    u_t16 = int(round(u * float(tex_w) * 16.0)) & 0xFFFF
    v_t16 = int(round(v * float(tex_h) * 16.0)) & 0xFFFF
    return (u_t16) | (v_t16 << 16)


# TODO: consider pack cmd function


def build_nds_display_list(triangles, tex_w, tex_h):
    """Compiles GL_TRIANGLES primitive data into NDS display list format.

    Args:
        triangles (list): List of tuples containing vertex and UV data for each triangle.
        tex_w (int): Width of the texture in pixels.
        tex_h (int): Height of the texture in pixels.
    Returns:
        list: A list of 32-bit integers representing the NDS display list commands and data.
    """
    dl_words = []

    # Command: BEGIN & GL_TRIANGLES
    dl_words.append(FIFO_BEGIN)
    dl_words.append(GL_TRIANGLES)

    for v_tri, uv_tri, col_tri in triangles:
        for j in range(3):
            if col_tri is not None and len(col_tri) > j:
                dl_words.append(FIFO_COLOR)
                dl_words.append(rgb_to_rgb15(col_tri[j]))

            if uv_tri is not None and len(uv_tri) > j:
                u, v = uv_tri[j]
                dl_words.append(FIFO_TEXCOORD)
                dl_words.append(pack_uv_t16(u, v, tex_w, tex_h))

            vx, vy, vz = v_tri[j]
            v16_x = float_to_v16(vx) & 0xFFFF
            v16_y = float_to_v16(vy) & 0xFFFF
            v16_z = float_to_v16(vz) & 0xFFFF

            dl_words.append(FIFO_VERTEX16)
            dl_words.append(v16_x | (v16_y << 16))
            dl_words.append(v16_z)

    # Command: END
    dl_words.append(FIFO_END)
    return dl_words


# Read Helpers


def get_prop(obj, key, default=None):
    """Helper to read attributes safely from either pygltflib objects or dicts.
    Args:
        obj: The object or dictionary to read from.
        key: The key or attribute name to retrieve.
        default: The default value to return if the key/attribute is not found.
    Returns:
        The value of the key/attribute."""
    if obj is None:
        return default
    if isinstance(obj, dict):  # Dictionary Check
        return obj.get(key, default)
    return getattr(obj, key, default)  # Object Check


def read_accessor_data(gltf, accessor_index):
    """Reads accessor data from GLTF and returns as a numpy array.

    Args:
        gltf: The GLTF object.
        accessor_index: The index of the accessor to read.
    Returns:
        np.ndarray: The data from the accessor as a numpy array."""
    if accessor_index is None:
        return None
    accessor = gltf.accessors[accessor_index]
    buffer_view = gltf.bufferViews[accessor.bufferView]

    # There are other ways to get the data, but they don't always work (seems to be based on the pygltflib version). This method worked in this scenario so we're going with that.
    data = (
        gltf.binary_blob()
        if callable(getattr(gltf, "binary_blob", None))
        else getattr(gltf, "binary_data", None)
    )
    if data is None:
        raise ValueError("Could not extract binary buffer data from GLB File")

    byte_offset = (buffer_view.byteOffset or 0) + (accessor.byteOffset or 0)
    dtype = DTYPE_MAP[accessor.componentType]
    count = accessor.count
    num_components = TYPE_COUNTS[accessor.type]

    raw_array = np.frombuffer(
        data[
            byte_offset : byte_offset
            + count * num_components * np.dtype(dtype).itemsize
        ],
        dtype=dtype,
    )
    if num_components > 1:
        return raw_array.reshape(count, num_components)
    return raw_array


def apply_texture_transform(uvs, transform):
    """Apply KHR_texture_transform (offset, scale, rotation) to UVs.

    Args:
        uvs (np.ndarray): The original UV coordinates.
        transform (dict): The KHR_texture_transform extension data.
    Returns:
        np.ndarray: The transformed UV coordinates."""
    if uvs is None or transform is None:
        return uvs

    offset = get_prop(transform, "offset", [0.0, 0.0]) or [0.0, 0.0]
    scale = get_prop(transform, "scale", [1.0, 1.0]) or [1.0, 1.0]
    rotation = float(get_prop(transform, "rotation", 0.0) or 0.0)

    # Small threshold to avoid unnecessary transformations
    if (
        abs(offset[0]) < 1e-8
        and abs(offset[1]) < 1e-8
        and abs(scale[0] - 1.0) < 1e-8
        and abs(scale[1] - 1.0) < 1e-8
        and abs(rotation) < 1e-8
    ):
        return uvs

    # Create a copy to avoid modifying the original array and then scale
    transformed = np.array(uvs, copy=True)
    transformed[:, 0] *= float(scale[0])
    transformed[:, 1] *= float(scale[1])

    # Apply rotation if necessary (small threshold to avoid unnecessary computation)
    if abs(rotation) >= 1e-8:
        cos_r = float(np.cos(rotation))
        sin_r = float(np.sin(rotation))
        u = transformed[:, 0].copy()
        v = transformed[:, 1].copy()
        transformed[:, 0] = (u * cos_r) - (v * sin_r)
        transformed[:, 1] = (u * sin_r) + (v * cos_r)

    # Apply translation offset
    transformed[:, 0] += float(offset[0])
    transformed[:, 1] += float(offset[1])
    return transformed


def process_texture_img(gltf, image_index, tmp_dir):
    """Extracts glTF image, resizes to NDS power-of-two dimensions, and runs GRIT on it.

    Args:
        gltf: The GLTF object.
        image_index (int): The index of the image to process.
        tmp_dir (str): Temporary directory for intermediate files.
    Returns:
        dict: A dictionary containing texture information (name, width, height, isRGBA).
    """
    # TODO Can we integrate this whole process into this python script (so no temp files are created)?
    # Currently we're extracting the image into a png file, converting it to a binary file, and then attaching it to our mdl file

    image = gltf.images[image_index]
    texture_base_name = f"tex_{image_index}"

    # Extract raw image bytes from GLTF
    buffer_view_index = get_prop(image, "bufferView")
    if buffer_view_index is not None:
        bv = gltf.bufferViews[buffer_view_index]
        buffer = gltf.buffers[get_prop(bv, "buffer")]
        blob = (
            gltf.binary_blob()
            if get_prop(buffer, "uri") is None
            else gltf.get_data_from_buffer_uri(get_prop(buffer, "uri"))
        )
        offset = get_prop(bv, "byteOffset", 0) or 0
        length = get_prop(bv, "byteLength")
        img_bytes = blob[offset : offset + length]

        # TODO: arg for non_temp output? for debugging
        # Save temporary input png
        temp_png_path = os.path.join(tmp_dir, f"tex_{image_index}.png")
        with open(temp_png_path, "wb") as f:
            f.write(img_bytes)
    else:
        raise ValueError(
            f"Image {image_index} has no bufferView; cannot extract image data."
        )

    # Enforce NDS texture size constraints (power-of-two)
    img = Image.open(temp_png_path).convert("RGBA")

    target_w = min(1024, next_pow2(img.width))
    target_h = min(1024, next_pow2(img.height))
    if img.width != target_w or img.height != target_h:
        img = img.resize((target_w, target_h), Image.Resample.LANCZOS)
        img.save(temp_png_path)

    # Run GRIT command to convert PNG to NDS texture format
    # -gt: Texture mode | -gB16: 16-bit A1BGR555 | -gT!: set alpha-bit  | -ftb: Binary output | -fh!: No C header
    temp_grit_output_base = os.path.join(tmp_dir, texture_base_name)
    grit_cmd = [
        GRIT_PATH,
        temp_png_path,
        "-gb",
        "-gB16",
        "-gT!",
        "-ftb",
        "-fh!",
        "-o",
        temp_grit_output_base,
    ]

    try:
        subprocess.run(
            grit_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print(f"Error running GRIT for image {image_index}: {e}")
        raise e

    texture_base_name = f"tex_{image_index}"
    tex_name = texture_base_name[:63].ljust(64, "\0")

    tex_data = {
        "name": tex_name,
        "w": target_w,
        "h": target_h,
        "isRGBA": 1,
    }
    with open(temp_grit_output_base + ".img.bin", "rb") as f:
        image_data = f.read()

    image = {
        "name": tex_name,
        "byteLength": len(image_data),
        "data": image_data,
    }

    return tex_data, image


# Core Functions
def construct_texture_table(gltf):
    """Constructs a texture table from the GLTF images and passes them to GRIT.
    Args:
        gltf: The GLTF object.
    Returns:
        list: A list of texture data.
        list: A list of image binary data.
    """
    textures = []
    images = []
    with tempfile.TemporaryDirectory() as tmp_dir:
        for index in range(len(gltf.images)):
            tex_info, image = process_texture_img(gltf, index, tmp_dir)
            textures.append(tex_info)
            images.append(image)
    return textures, images


def write_mdl_file(output_path, nodes, textures, images, animations=[]):
    """Write extracted data to binary MDL3 file.

    Args:
        output_path (str): Path to the output MDL3 file.
        nodes (list): List of node dictionaries containing position and sublist data.
        textures (list): List of texture dictionaries containing name, width, height, and RGBA flag.
        images (list): Images dictionaries containing name, byte length, and binary data.
        animations (list): List of animation dictionaries containing name, number of frames, fps, and tracks.
    """
    with open(output_path, "wb") as f:
        # Header: 'MDL3' | u32 nodeCount | u32 animCount | u32 texCount
        f.write(
            struct.pack("<4sIII", MAGIC, len(nodes), len(animations), len(textures))
        )

        # Tex Table
        for tex in textures:
            f.write(
                struct.pack(
                    "<64sHHB3s",
                    tex["name"].encode("ascii"),
                    tex["w"],
                    tex["h"],
                    tex["isRGBA"],
                    b"\0\0\0",
                )
            )

        for node in nodes:
            # Node Header: s32 pid, s32 px, 32 py, s32 pz, u32 subListCount
            f.write(
                struct.pack(
                    "<iiiiI",
                    node["pid"],
                    node["px"],
                    node["py"],
                    node["pz"],
                    node["subListCount"],
                )
            )
            for sub_list in node["subLists"]:
                f.write(struct.pack("<iI", sub_list["texSlot"], sub_list["dlSize"]))
                for word in sub_list["dlWords"]:
                    f.write(struct.pack("<I", word))

        if animations != [] and animations is not None and len(animations) > 0:
            # Animations
            for anim in animations:
                # 1. Anim Header: 32-byte name | u32 num_frames | s16 fps
                name_bytes = anim["name"].encode("ascii")[:31].ljust(32, b"\0")
                f.write(
                    struct.pack(
                        "<32sIh",
                        name_bytes,
                        anim["num_frames"],
                        float_to_s16(anim["fps"]),
                    )
                )

                # 2. Track Count: u32 trackCount
                tracks = anim["tracks"]
                f.write(struct.pack("<I", len(tracks)))

                # 3. Individual Tracks
                for track in tracks:
                    # Node Index
                    f.write(struct.pack("<i", track["nodeIndex"]))
                    f.write(struct.pack("<I", len(track["t"])))

                    # Translation Keys: u32 count -> s16 time, s32 x, s32 y, s32 z
                    for k in track["t"]:
                        val = k["val"]
                        f.write(
                            struct.pack(
                                "<hiii",
                                float_to_s16(k["time"]),
                                int(val[0] * 0.25),
                                int(val[1] * 0.25),
                                int(val[2] * 0.25),
                            )
                        )

                    # Rotation Keys: u32 count -> s16 time, s16 x, s16 y, s16 z, s16 w
                    f.write(struct.pack("<I", len(track["r"])))
                    for k in track["r"]:
                        val = k["val"]
                        f.write(
                            struct.pack(
                                "<hhhhh",
                                float_to_s16(k["time"]),
                                quat_float_to_s16(val[0]),
                                quat_float_to_s16(val[1]),
                                quat_float_to_s16(val[2]),
                                quat_float_to_s16(val[3]),
                            )
                        )

        # Write texture binary data to end of file
        for image in images:
            # Anim Header: 32-byte name | u32 byteLength
            tex_name_bytes = tex["name"].encode("ascii")[:31].ljust(32, b"\0")
            f.write(struct.pack("<32sI", tex_name_bytes, image["byteLength"]))
            f.write(image["data"])

        print(
            f"Outputted '{output_path}' ({len(nodes)} nodes, {len(textures)} textures, {len(animations)} animations)."
        )
