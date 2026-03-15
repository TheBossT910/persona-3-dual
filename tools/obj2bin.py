import sys
import struct
import argparse

FIFO_COLOR    = 0x20
FIFO_TEXCOORD = 0x22
FIFO_BEGIN    = 0x40
FIFO_VERTEX16 = 0x23
FIFO_NOP      = 0x00

GL_TRIANGLES = 0
GL_QUADS     = 1

# Valid DS texture dimensions from GL_TEXTURE_SIZE_ENUM
VALID_SIZES = {8, 16, 32, 64, 128, 256, 512, 1024}

def floattov16(f):
    v = int(f * (1 << 12))
    return max(-32768, min(32767, v)) & 0xFFFF

def floattot16(f):
    """t16 is 12.4 fixed point in TEXELS, not normalized coords.
       Input here should already be pre-multiplied by texture dimension.
    """
    v = int(f * (1 << 4))
    return max(-32768, min(32767, v)) & 0xFFFF

def rgb_to_rgb15(r, g, b):
    return ((r >> 3) & 0x1F) | (((g >> 3) & 0x1F) << 5) | (((b >> 3) & 0x1F) << 10)

def pack_cmds(c1, c2=FIFO_NOP, c3=FIFO_NOP, c4=FIFO_NOP):
    return struct.pack('<I', (c4 << 24) | (c3 << 16) | (c2 << 8) | c1)

def convert_obj(input_file, output_file, tex_width=None, tex_height=None, vertex_color=None):
    vertices = []
    texcoords = []
    faces = []

    with open(input_file, 'r') as f:
        for line in f:
            parts = line.split()
            if not parts:
                continue
            if parts[0] == 'v':
                vertices.append((float(parts[1]), float(parts[2]), float(parts[3])))
            elif parts[0] == 'vt':
                texcoords.append((float(parts[1]), float(parts[2])))
            elif parts[0] == 'f':
                face = []
                for p in parts[1:]:
                    components = p.split('/')
                    v_idx  = int(components[0]) - 1
                    vt_idx = int(components[1]) - 1 if len(components) > 1 and components[1] != '' else None
                    face.append((v_idx, vt_idx))
                faces.append(face)

    has_uvs = any(vt is not None for face in faces for _, vt in face)

    if has_uvs and (tex_width is None or tex_height is None):
        print("Error: this .obj has UV coordinates but no --texsize was specified.")
        print("UV coordinates are in texels on the DS, so the texture dimensions are required.")
        print("Example: --texsize 128 128")
        sys.exit(1)

    if has_uvs:
        if tex_width not in VALID_SIZES or tex_height not in VALID_SIZES:
            print(f"Error: texture dimensions must be powers of 2 between 8 and 1024.")
            print(f"Valid sizes: {sorted(VALID_SIZES)}")
            sys.exit(1)

    words = []

    if vertex_color is not None:
        r, g, b = vertex_color
        words.append(pack_cmds(FIFO_COLOR))
        words.append(struct.pack('<I', rgb_to_rgb15(r, g, b)))

    for face in faces:
        if len(face) == 4:
            prim_type = GL_QUADS
        elif len(face) == 3:
            prim_type = GL_TRIANGLES
        else:
            continue

        words.append(pack_cmds(FIFO_BEGIN))
        words.append(struct.pack('<I', prim_type))

        for v_idx, vt_idx in face:
            if has_uvs and vt_idx is not None:
                u, v = texcoords[vt_idx]
                # Scale normalized 0-1 UVs to texel space before t16 conversion.
                # .obj V axis is flipped relative to DS (0=bottom in .obj, 0=top on DS)
                u16   = floattot16(u * tex_width)
                v16   = floattot16((1.0 - v) * tex_height)
                words.append(pack_cmds(FIFO_TEXCOORD))
                words.append(struct.pack('<I', (u16 & 0xFFFF) | ((v16 & 0xFFFF) << 16)))

            vert = vertices[v_idx]
            words.append(pack_cmds(FIFO_VERTEX16))
            words.append(struct.pack('<I', (floattov16(vert[1]) << 16) | floattov16(vert[0])))
            words.append(struct.pack('<I', floattov16(vert[2])))

    with open(output_file, 'wb') as out:
        out.write(struct.pack('<I', len(words)))
        for w in words:
            out.write(w)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Convert OBJ to NDS display list binary')
    parser.add_argument('input',  help='Input .obj file')
    parser.add_argument('output', help='Output .bin file')
    parser.add_argument('--texsize', nargs=2, type=int, metavar=('W', 'H'),
                        help='Texture dimensions in pixels, e.g. --texsize 128 128')
    parser.add_argument('--color', nargs=3, type=int, metavar=('R', 'G', 'B'),
                        help='Flat vertex color 0-255, e.g. --color 255 128 0')
    args = parser.parse_args()

    tw, th = (args.texsize[0], args.texsize[1]) if args.texsize else (None, None)
    color  = tuple(args.color) if args.color else None

    convert_obj(args.input, args.output, tex_width=tw, tex_height=th, vertex_color=color)
    print(f"Converted {args.input} -> {args.output}")