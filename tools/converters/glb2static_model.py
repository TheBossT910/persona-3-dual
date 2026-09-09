import sys

import pygltflib
import numpy as np

from glb_utils import (
    get_prop,
    read_accessor_data,
    build_nds_display_list,
    apply_texture_transform,
    construct_texture_table,
    write_mdl_file,
)

MAGIC = b"MDLS"

# Helpers


def transform_positions(positions, matrix):
    """Transforms local Nx3 positions into world space.

    Args:
        positions: Array of shape (N, 3) representing vertex positions.
        matrix: 4x4 transformation matrix.
    Returns:
        Array of shape (N, 3) representing transformed vertex positions."""
    ones = np.ones((len(positions), 1), dtype=np.float32)
    pos_homo = np.hstack([positions, ones])
    return (matrix @ pos_homo.T).T[:, :3]


def get_node_local_matrix(node):
    """Builds a 4x4 matrix from glTF node translation, rotation, and scale.
    Args:
        node: The glTF node object.
    Returns:
        A 4x4 transformation matrix."""
    # Extract properties
    t = getattr(node, "translation", None) or [0.0, 0.0, 0.0]
    q = getattr(node, "rotation", None) or [0.0, 0.0, 0.0, 1.0]
    s = getattr(node, "scale", None) or [1.0, 1.0, 1.0]

    # Guard against zero scale
    if s[0] == 0.0 and s[1] == 0.0 and s[2] == 0.0:
        s = [1.0, 1.0, 1.0]

    # Construct the component matrices
    x, y, z, w = q
    R = np.array(
        [
            [1 - 2 * (y**2 + z**2), 2 * (x * y - z * w), 2 * (x * z + y * w), 0],
            [2 * (x * y + z * w), 1 - 2 * (x**2 + z**2), 2 * (y * z - x * w), 0],
            [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x**2 + y**2), 0],
            [0, 0, 0, 1],
        ],
        dtype=np.float32,
    )
    T = np.eye(4, dtype=np.float32)
    T[:3, 3] = t
    S = np.diag([s[0], s[1], s[2], 1.0]).astype(np.float32)

    # Combine via matrix multiplication: M = T * R * S
    return T @ R @ S


def compute_global_matrices(gltf):
    """Computes final world matrices for every node in the scene.

    Args:
        gltf: The glTF object.
    Returns:
        A list of 4x4 transformation matrices for each node."""
    nodes = gltf.nodes
    parents = {i: None for i in range(len(nodes))}
    for i, node in enumerate(nodes):
        for child in get_prop(node, "children", []):
            parents[child] = i

    def get_global_mat(index):
        """Recursively computes the global transformation matrix for a node.

        Args:
            index: The index of the node in the glTF nodes list.
        Returns:
            A 4x4 transformation matrix representing the node's world transform."""
        local_mat = get_node_local_matrix(nodes[index])
        parent_index = parents[index]
        return (
            get_global_mat(parent_index) @ local_mat
            if parent_index is not None
            else local_mat
        )

    return [get_global_mat(i) for i in range(len(nodes))]


def resolve_primitive_material(gltf, primitive):
    """Resolves the material properties of a glTF primitive.

    Args:
        gltf: The glTF object.
        primitive: The glTF primitive object.
    Returns:
        A tuple (tex_slot, uvs) where tex_slot is the index of the texture in the texture table, and uvs is an Nx2 array of UV coordinates.
    """
    gltf_textures = get_prop(gltf, "textures", [])
    tex_slot = -1
    uv_set = 0
    texture_transform = None

    # Resolve the material index and associated texture information
    mat_index = get_prop(primitive, "material", None)
    if (
        mat_index is not None
        and hasattr(gltf, "materials")
        and gltf.materials
        and mat_index < len(gltf.materials)
    ):
        mat = gltf.materials[mat_index]
        pbr = get_prop(mat, "pbrMetallicRoughness", None) or {}
        base_tex = get_prop(pbr, "baseColorTexture", None) or {}
        uv_set = int(get_prop(base_tex, "texCoord", 0) or 0)
        extensions = get_prop(base_tex, "extensions", None) or {}
        texture_transform = get_prop(extensions, "KHR_texture_transform", None)
        if texture_transform is not None:
            uv_set = int(get_prop(texture_transform, "texCoord", uv_set) or uv_set)

        # Resolve the texture index and source image
        gltf_tex_index = get_prop(base_tex, "index", -1)
        if gltf_tex_index is not None and 0 <= gltf_tex_index < len(gltf_textures):
            src_img = get_prop(gltf_textures[gltf_tex_index], "source", None)
            if src_img is not None:
                tex_slot = src_img

    # Extract UV data and apply texture transform if present
    attrs = get_prop(primitive, "attributes", {})
    uv_index = get_prop(attrs, f"TEXCOORD_{uv_set}", None)
    raw_uvs = read_accessor_data(gltf, uv_index) if uv_index is not None else None
    uvs = apply_texture_transform(raw_uvs, texture_transform)

    return tex_slot, uvs


def extract_primitive_triangles(positions, uvs, colors, indices):
    """Extracts triangles from a glTF primitive's vertex data.

    Args:
        positions: Array of vertex positions.
        uvs: Array of vertex UVs.
        colors: Array of vertex colors.
        indices: Array of vertex indices.
    Returns:
        A list of triangles, where each triangle is a tuple (v_tri, uv_tri, color_tri).
    """
    if positions is None or len(positions) == 0:
        return []

    # Handle indexed vs unindexed geometry streams
    if indices is not None and len(indices) > 0:
        flat_indices = np.asarray(indices).flatten()
    else:
        flat_indices = np.arange(len(positions))

    triangles = []
    num_indices = len(flat_indices)

    # Process vertices in strides of 3 (gltf TRIANGLES primitive mode)
    for i in range(0, num_indices - 2, 3):
        index0 = flat_indices[i]
        index1 = flat_indices[i + 1]
        index2 = flat_indices[i + 2]

        v_tri = [positions[index0], positions[index1], positions[index2]]

        uv_tri = (
            [uvs[index0], uvs[index1], uvs[index2]]
            if uvs is not None and len(uvs) > max(index0, index1, index2)
            else None
        )

        color_tri = (
            [colors[index0], colors[index1], colors[index2]]
            if colors is not None and len(colors) > max(index0, index1, index2)
            else None
        )

        triangles.append((v_tri, uv_tri, color_tri))

    return triangles


# Core Functions


def parse_mesh(gltf, textures):
    """Parses the glTF mesh hierarchy and constructs display lists.

    Args:
        gltf: The glTF object.
        textures: List of texture information.
    Returns:
        A list of display lists for the mesh."""
    global_matrices = compute_global_matrices(gltf)
    tex_batches = {}

    for node_index, node in enumerate(gltf.nodes):
        mesh_id = get_prop(node, "mesh", None)
        if mesh_id is None or mesh_id >= len(gltf.meshes):
            continue

        # Get the world matrix for this node so we can transform the mesh to world space
        world_matrix = global_matrices[node_index]

        mesh = gltf.meshes[mesh_id]

        for primitive in get_prop(mesh, "primitives", []):
            attrs = get_prop(primitive, "attributes", {})
            pos_accessor = (
                get_prop(attrs, "POSITION", None)
                if not isinstance(attrs, dict)
                else attrs.get("POSITION")
            )
            positions = read_accessor_data(gltf, pos_accessor)
            indices_accessor = get_prop(primitive, "indices", None)
            indices = read_accessor_data(gltf, indices_accessor)

            # Extract baked vertex colors (COLOR_0)
            color_index = get_prop(attrs, "COLOR_0", None)
            colors = (
                read_accessor_data(gltf, color_index)
                if color_index is not None
                else None
            )

            world_positions = transform_positions(positions, world_matrix) * 160
            tex_slot, uvs = resolve_primitive_material(gltf, primitive)

            triangles = extract_primitive_triangles(
                world_positions, uvs, colors, indices
            )
            tex_batches.setdefault(tex_slot, []).extend(triangles)

    # Build display lists for each texture batch
    sub_lists = []
    for tex_slot, triangles in tex_batches.items():
        tex_w = textures[tex_slot]["w"] if 0 <= tex_slot < len(textures) else 128
        tex_h = textures[tex_slot]["h"] if 0 <= tex_slot < len(textures) else 128

        dl_words = build_nds_display_list(triangles, tex_w, tex_h)
        sub_lists.append(
            {"texSlot": tex_slot, "dlSize": len(dl_words), "dlWords": dl_words}
        )

    return [
        {
            "pid": -1,
            "px": 0,
            "py": 0,
            "pz": 0,
            "subListCount": len(sub_lists),
            "subLists": sub_lists,
        }
    ]


def convert_glb_to_mdls(input_glb: str, output_path: str):
    """Converts a GLB file to the custom MDL3 format, for static meshes.

    Args:
        input_glb (str): Path to the input GLB file.
        output_path (str): Path to the output MDL3 file.
        rgba (bool): Whether to export images in RGBA format.
    """
    gltf = pygltflib.GLTF2().load(input_glb)

    # Textures Table
    textures, images = construct_texture_table(gltf)

    # Parse Mesh hierarchy and construct display list
    nodes = parse_mesh(gltf, textures)

    # Write to the MDL3 file
    write_mdl_file(output_path, nodes, textures, images)


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python glb2mdls.py <input.glb> <output.mdl3>")
        sys.exit(1)

    input_glb = sys.argv[1]
    output_path = sys.argv[2]

    convert_glb_to_mdls(input_glb, output_path)
