import sys
import re

import numpy as np
import pygltflib

from glb_utils import (
    get_prop,
    read_accessor_data,
    float_to_v16,
    build_nds_display_list,
    construct_texture_table,
    apply_texture_transform,
    write_mdl_file,
)

FRAME_RATE = 30.0

# Mesh Functions


def unskin_primitive(gltf, primitive, skin, positions, uvs, colors, indices):
    """Splits skinned mesh vertices by primary bone and converts them to bone local space.

    Args:
        gltf: The GLTF object.
        primitive: The mesh primitive to unskin.
        skin: The skin object associated with the primitive.
        positions (np.ndarray): Vertex positions.
        uvs (np.ndarray): Vertex UV coordinates.
        colors (np.ndarray): Vertex colors.
        indices (np.ndarray): Triangle indices.
    Returns:
        dict: A dictionary mapping joint node indices to lists of triangles (each triangle is a tuple of vertex positions and UVs).
    """
    ibm_accessor = get_prop(skin, "inverseBindMatrices")
    ibm_raw = (
        read_accessor_data(gltf, ibm_accessor) if ibm_accessor is not None else None
    )
    joints = get_prop(skin, "joints", [])

    attrs = get_prop(primitive, "attributes", {})
    joints_index = get_prop(attrs, "JOINTS_0")
    weights_index = get_prop(attrs, "WEIGHTS_0")

    joints_data = read_accessor_data(gltf, joints_index)
    weights_data = read_accessor_data(gltf, weights_index)

    if joints_data is None or weights_data is None:
        print(
            "Warning: Skinned primitive missing JOINTS_0 or WEIGHTS_0; skipping unskinning."
        )
        return None

    # Initialize inverse bind matrices (IBMs) for each joint
    num_joints = len(joints)
    if ibm_raw is not None:
        ibms = ibm_raw.reshape(num_joints, 4, 4)
    else:
        ibms = np.tile(np.eye(4, dtype=np.float32), (num_joints, 1, 1))

    # Find bone with the maximum skin weight for each vertex
    dominant_sub_indices = np.argmax(weights_data, axis=1)

    flat_indices = (
        indices.flatten() if indices is not None else np.arange(len(positions))
    )
    joint_triangles = {j_node: [] for j_node in joints}

    for i in range(0, len(flat_indices), 3):
        index0 = flat_indices[i]
        index1 = flat_indices[i + 1]
        index2 = flat_indices[i + 2]

        # Use vertex 0's primary bone as triangle owner
        sub_j_index = joints_data[index0, dominant_sub_indices[index0]]
        target_node = joints[sub_j_index]

        # gltf matrices are column-major, so transpose for numpy
        ibm = ibms[sub_j_index].T

        v_tri = []
        uv_tri = []
        col_tri = []
        for v_index in (index0, index1, index2):
            pos = positions[v_index]
            pos_h = np.array([pos[0], pos[1], pos[2], 1.0], dtype=np.float32)
            local_pos = (ibm @ pos_h)[
                :3
            ] * 0.25  # TODO: dynamic scale factor? i just picked this to get a working demo
            v_tri.append(local_pos)
            if uvs is not None:
                uv_tri.append(uvs[v_index])
            if colors is not None:
                col_tri.append(colors[v_index])

        joint_triangles[target_node].append(
            (
                v_tri,
                uv_tri if uvs is not None else None,
                col_tri if colors is not None else None,
            )
        )

    # Check for discarded triangles due to unskinning
    total_input_tris = len(flat_indices) // 3
    total_output_tris = sum(len(tris) for tris in joint_triangles.values())

    if total_input_tris != total_output_tris:
        print(
            f"  WARNING: Discarded {total_input_tris - total_output_tris} triangles during unskinning!"
        )
    return joint_triangles


# Core Functions


def parse_mesh_primitives(gltf, textures):
    """Parses mesh primitives from the GLTF and constructs display lists for each node.

    Args:
        gltf: The GLTF object.
        textures (list): A list of texture data.
    Returns:
        list: A list of sublists for each node, where each sublist contains display list data for the node's primitives.
    """
    gltf_textures = get_prop(gltf, "textures", []) or []
    tex_count = len(textures)

    node_sub_lists = [[] for _ in range(len(gltf.nodes))]
    for index, node in enumerate(gltf.nodes):
        mesh_id = get_prop(node, "mesh", None)
        skin_id = get_prop(node, "skin", None)

        if mesh_id is not None and mesh_id < len(gltf.meshes):
            mesh = gltf.meshes[mesh_id]
            primitives = get_prop(mesh, "primitives", [])
            skin = (
                gltf.skins[skin_id]
                if (skin_id is not None and skin_id < len(gltf.skins))
                else None
            )

            for prim in primitives:
                attrs = get_prop(prim, "attributes", {})
                pos_index = get_prop(attrs, "POSITION")
                color_index = get_prop(attrs, "COLOR_0")  # Extract COLOR_0 index
                indices_index = get_prop(prim, "indices")
                mat_index = get_prop(prim, "material")

                positions = read_accessor_data(gltf, pos_index)
                colors = read_accessor_data(gltf, color_index)  # Read COLOR_0 accessor
                indices = read_accessor_data(gltf, indices_index)

                # Resolve Texture
                tex_slot = -1
                uv_set = 0
                texture_transform = None
                if mat_index is not None and mat_index < len(gltf.materials):
                    mat = gltf.materials[mat_index]
                    pbr = get_prop(mat, "pbrMetallicRoughness", {})
                    base_tex = get_prop(pbr, "baseColorTexture", {})
                    uv_set = int(get_prop(base_tex, "texCoord", 0) or 0)
                    extension = get_prop(mat, "extensions", {}) or {}
                    texture_transform = get_prop(
                        extension, "KHR_texture_transform", None
                    )
                    if texture_transform is not None:
                        uv_set = int(
                            get_prop(texture_transform, "texCoord", uv_set) or uv_set
                        )
                    gltf_tex_index = get_prop(base_tex, "index", -1)
                    if gltf_tex_index is not None and 0 <= gltf_tex_index < len(
                        gltf.textures
                    ):
                        gltf_tex = gltf_textures[gltf_tex_index]
                        source_image_index = get_prop(gltf_tex, "source", -1)
                        if source_image_index is not None:
                            tex_slot = source_image_index

                uv_index = get_prop(attrs, f"TEXCOORD_{uv_set}")
                uvs = read_accessor_data(gltf, uv_index)
                uvs = apply_texture_transform(uvs, texture_transform)

                tex_w = (
                    textures[tex_slot]["w"]
                    if tex_slot >= 0 and tex_slot < tex_count
                    else 128
                )
                tex_h = (
                    textures[tex_slot]["h"]
                    if tex_slot >= 0 and tex_slot < tex_count
                    else 128
                )

                # Pass colors to unskin_primitive
                joint_triangles = (
                    unskin_primitive(gltf, prim, skin, positions, uvs, colors, indices)
                    if skin
                    else None
                )

                if joint_triangles:
                    for joint_node_index, triangles in joint_triangles.items():
                        if not triangles:
                            continue
                        dl_words = build_nds_display_list(triangles, tex_w, tex_h)
                        node_sub_lists[joint_node_index].append(
                            {
                                "texSlot": tex_slot,
                                "dlSize": len(dl_words),
                                "dlWords": dl_words,
                            }
                        )
                else:  # Rigid primitive; build display list directly for this node
                    flat_indices = (
                        indices.flatten()
                        if indices is not None
                        else np.arange(len(positions))
                    )
                    triangles = []
                    for i in range(0, len(flat_indices), 3):
                        v_tri = [positions[flat_indices[i + j]] for j in range(3)]
                        uv_tri = (
                            [uvs[flat_indices[i + j]] for j in range(3)]
                            if uvs is not None
                            else None
                        )
                        col_tri = (
                            [colors[flat_indices[i + j]] for j in range(3)]
                            if colors is not None
                            else None
                        )
                        triangles.append((v_tri, uv_tri, col_tri))

                    dl_words = build_nds_display_list(triangles, tex_w, tex_h)
                    node_sub_lists[index].append(
                        {
                            "texSlot": tex_slot,
                            "dlSize": len(dl_words),
                            "dlWords": dl_words,
                        }
                    )
    return node_sub_lists


def construct_nodes(gltf, node_sub_lists):
    """Constructs a list of node dictionaries containing position and sublist data.

    Args:
        gltf: The GLTF object.
        node_sub_lists (list): A list of sublists for each node.
    Returns:
        list: A list of node dictionaries containing parent index, position, sublist count, and sublists.
    """
    nodes = []
    for index, node in enumerate(gltf.nodes):
        # Find parent node index
        pid = -1
        for parent_index, parent_node in enumerate(gltf.nodes):
            children = get_prop(parent_node, "children", [])
            if children and index in children:
                pid = parent_index
                break

        translation = get_prop(node, "translation", [0.0, 0.0, 0.0])
        px, py, pz = (
            [float_to_v16(v) for v in translation] if translation else (0, 0, 0)
        )
        sub_lists = node_sub_lists[index]

        nodes.append(
            {
                "pid": pid,
                "px": px,
                "py": py,
                "pz": pz,
                "subListCount": len(sub_lists),
                "subLists": sub_lists,
            }
        )
    return nodes


def parse_animations(gltf, node_count):
    """Extracts keyframe channels per node for translation, rotation and scaling.

    Args:
        gltf: The GLTF object.
        node_count (int): The number of nodes in the GLTF scene.
    Returns:
        list: A list of animation dictionaries, each containing name, number of frames, fps, and tracks (per node keyframe data).
    """
    animations = []
    if not hasattr(gltf, "animations") or gltf.animations is None:
        return animations

    for anim_index, anim in enumerate(gltf.animations):
        # Ensure string is always used, even if name is None
        raw_name = get_prop(anim, "name")
        anim_name = raw_name if raw_name is not None else f"anim_{anim_index}"
        # Match C++ 32-byte name length
        clean_name = re.sub(r"[^a-zA-Z0-9_]", "_", anim_name)[:31].ljust(32, "\0")

        # Track per node containing keyframe lists
        node_tracks = [{"nodeIndex": i, "t": [], "r": []} for i in range(node_count)]
        max_duration = 0.0

        channels = get_prop(anim, "channels", [])
        samplers = get_prop(anim, "samplers", [])

        for channel in channels:
            target = get_prop(channel, "target", {})
            target_node = get_prop(target, "node")
            path = get_prop(target, "path")

            if target_node is None or target_node >= node_count:
                continue

            sampler_index = get_prop(channel, "sampler")
            sampler = samplers[sampler_index]

            times = read_accessor_data(gltf, get_prop(sampler, "input"))
            values = read_accessor_data(gltf, get_prop(sampler, "output"))

            if times is not None and values is not None:
                # Sort the keyframes chronologically by time to ensure correct order
                combined = sorted(zip(times, values), key=lambda x: x[0])
                times, values = zip(*combined) if combined else ([], [])

            if times is None or values is None:
                continue

            if len(times) > 0 and times[-1] > max_duration:
                max_duration = float(times[-1])

            num_frames = int(max_duration * FRAME_RATE)

            keys = []
            for t, val in zip(times, values):
                frame_num = float(t) * FRAME_RATE

                if path == "translation":
                    # Scale position vectors (meters) by 4096.0 to match base node 4.12 fixed-point units
                    scaled_val = [float(v) * 4096.0 for v in val]
                    keys.append({"time": frame_num, "val": scaled_val})
                else:
                    # Rotations (quaternions) MUST remain raw floats in the [-1.0, 1.0] range
                    keys.append({"time": frame_num, "val": val})
            if path == "translation":
                node_tracks[target_node]["t"] = keys
            elif path == "rotation":
                node_tracks[target_node]["r"] = keys

        # Filter out tracks that have no keyframes to save memory/space
        active_tracks = [tr for tr in node_tracks if tr["t"] or tr["r"]]

        animations.append(
            {
                "name": clean_name,
                "num_frames": num_frames,
                "fps": FRAME_RATE,
                "tracks": active_tracks,
            }
        )

    return animations


def convert_glb_to_mdla(glb_path: str, output_path: str):
    """Converts a GLB file to a custom MDL3 format as an animated mesh.

    Args:
        glb_path (str): Path to the input GLB file.
        output_path (str): Path to the output MDL3 file.
    """
    gltf = pygltflib.GLTF2().load(glb_path)

    if not hasattr(gltf, "animations"):
        raise ValueError("Error: No animations found in the GLB file.")

    # Textures table
    textures, images = construct_texture_table(gltf)

    # Parse Mesh Primitives and Unskin if necessary
    node_sub_lists = parse_mesh_primitives(gltf, textures)

    # Construct binary base nodes
    nodes = construct_nodes(gltf, node_sub_lists)

    # Animations
    animations = parse_animations(gltf, len(nodes))

    write_mdl_file(output_path, nodes, textures, images, animations)


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python glb2model.py <input.glb> <output.mdl3>")
        sys.exit(1)

    input_glb = sys.argv[1]
    output_mdla = sys.argv[2]

    convert_glb_to_mdla(input_glb, output_mdla)
