import sys

def convert_obj(input_file, output_file):
    vertices = []
    faces = []

    # Read the .obj file
    with open(input_file, 'r') as f:
        for line in f:
            parts = line.split()
            if not parts: continue
            
            # Store vertices (x, y, z)
            if parts[0] == 'v':
                vertices.append((float(parts[1]), float(parts[2]), float(parts[3])))
            
            # Store faces (subtract 1 because .obj indices start at 1, not 0)
            elif parts[0] == 'f':
                # Grab just the vertex index, ignoring UV/Normal data for now
                face_verts = [int(p.split('/')[0]) - 1 for p in parts[1:]]
                faces.append(face_verts)

    # Write the C header file
    with open(output_file, 'w') as out:
        out.write("#include <nds.h>\n\n")
        out.write("void draw_custom_model() {\n")
        
        for face in faces:
            # The DS loves Quads!
            if len(face) == 4:
                out.write("    glBegin(GL_QUADS);\n")
            elif len(face) == 3:
                out.write("    glBegin(GL_TRIANGLES);\n")
            else:
                continue # Skip complex polygons

            # Write the fixed-point vertices
            for v_idx in face:
                v = vertices[v_idx]
                out.write(f"    glVertex3v16(floattov16({v[0]}), floattov16({v[1]}), floattov16({v[2]}));\n")
            
            out.write("    glEnd();\n")
        
        out.write("}\n")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python obj2c.py <input.obj> <output.h>")
    else:
        convert_obj(sys.argv[1], sys.argv[2])
        print(f"Successfully converted {sys.argv[1]} to {sys.argv[2]}")