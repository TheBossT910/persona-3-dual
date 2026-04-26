#!/usr/bin/env python3
"""
obj2offsets.py — Auto-calculate DS collision constants from a .obj + collision map.

Reads vertex bounds from a .obj file and the pixel dimensions of its paired
collision map PNG, then emits the three C defines you need:

    #define TILE_SIZE       0.0625f
    #define WORLD_OFFSET_X  2.0f
    #define WORLD_OFFSET_Z  2.0f

Usage:
    python3 obj2offsets.py scene.obj                       # prints to stdout
    python3 obj2offsets.py scene.obj --map map.png         # use explicit map PNG
    python3 obj2offsets.py scene.obj -o scene_offsets.h   # write header file
    python3 obj2offsets.py scene.obj --map map.png -o scene_offsets.h

The map PNG dimensions are used as the tile count (width = tile columns,
height = tile rows). If --map is omitted the script looks for a PNG with the
same base name as the .obj in assets/maps/, then falls back to asking you.
"""

import argparse
import os
import struct
import sys

# ── Optional PIL for PNG size ──────────────────────────────────────────────────
try:
    from PIL import Image as _PIL_Image
    _HAS_PIL = True
except ImportError:
    _HAS_PIL = False


# ── Helpers ────────────────────────────────────────────────────────────────────

def get_obj_bounds(obj_path: str) -> tuple[float, float, float, float]:
    """Return (minX, maxX, minZ, maxZ) from the vertex list of a .obj file."""
    min_x = min_z =  float("inf")
    max_x = max_z = -float("inf")
    found = False

    with open(obj_path, "r") as f:
        for line in f:
            parts = line.split()
            if not parts or parts[0] != "v":
                continue
            if len(parts) < 4:
                continue
            x = float(parts[1])
            z = float(parts[3])
            if x < min_x: min_x = x
            if x > max_x: max_x = x
            if z < min_z: min_z = z
            if z > max_z: max_z = z
            found = True

    if not found:
        print(f"ERROR: No vertices found in '{obj_path}'.", file=sys.stderr)
        sys.exit(1)

    return min_x, max_x, min_z, max_z


def png_dimensions_pil(png_path: str) -> tuple[int, int]:
    img = _PIL_Image.open(png_path)
    return img.size  # (width, height)


def png_dimensions_raw(png_path: str) -> tuple[int, int]:
    """Read PNG dimensions from the IHDR chunk without PIL."""
    with open(png_path, "rb") as f:
        sig = f.read(8)
        if sig != b"\x89PNG\r\n\x1a\n":
            raise ValueError("Not a valid PNG file.")
        f.read(4)  # chunk length
        chunk_type = f.read(4)
        if chunk_type != b"IHDR":
            raise ValueError("First chunk is not IHDR.")
        width  = struct.unpack(">I", f.read(4))[0]
        height = struct.unpack(">I", f.read(4))[0]
    return width, height


def crop_from_filename(png_path: str):
    """
    Extract crop rect (x, y, w, h) encoded in the filename as _X_Y_W_H.
    e.g. iwatodaiDorm_0_0_64_64.png -> (0, 0, 64, 64). Returns None if absent.
    """
    import re as _re
    stem = os.path.splitext(os.path.basename(png_path))[0]
    nums = _re.findall(r"_(\d+)", stem)
    if len(nums) >= 4:
        x, y, w, h = (int(n) for n in nums[-4:])
        return x, y, w, h
    return None


def get_png_size(png_path: str) -> tuple[int, int]:
    """
    Return the effective tile grid size for a collision map PNG.
    If the filename encodes a crop rect (_X_Y_W_H), return (w, h) from that
    instead of the full image dimensions.
    """
    crop = crop_from_filename(png_path)
    if crop:
        _, _, w, h = crop
        print(f"  Crop rect from filename: x={crop[0]}, y={crop[1]}, w={w}, h={h}")
        return w, h
    if _HAS_PIL:
        return png_dimensions_pil(png_path)
    return png_dimensions_raw(png_path)


def find_map_for_obj(obj_path: str) -> str | None:
    """
    Search heuristic:
      1. assets/maps/<stem>.png
      2. assets/maps/<stem>_*.png  (any crop-encoded variant, sorted)
      3. Same directory as .obj, <stem>.png
    """
    stem = os.path.splitext(os.path.basename(obj_path))[0]
    import re
    stem_plain = re.sub(r"_[0-9]+x[0-9]+$", "", stem)

    search_dirs = [
        os.path.join(os.path.dirname(obj_path), "..", "..", "assets", "maps"),
        os.path.join(os.path.dirname(obj_path), "..", "maps"),
        os.path.dirname(obj_path),
    ]

    for d in search_dirs:
        d = os.path.normpath(d)
        if not os.path.isdir(d):
            continue
        candidate = os.path.join(d, f"{stem_plain}.png")
        if os.path.isfile(candidate):
            return candidate
        for name in sorted(os.listdir(d)):
            if name.startswith(stem_plain) and name.endswith(".png"):
                return os.path.join(d, name)

    return None


def compute_offsets(min_x, max_x, min_z, max_z, tile_cols, tile_rows):
    offset_x    = -min_x
    offset_z    = -min_z
    
    # Engine Scale Fixed Rule: 1.0 world unit = 16 collision tiles.
    # We hardcode TILE_SIZE to 0.0625 (1/16). 
    # Do NOT calculate this dynamically from the .obj bounds (world_width / tile_cols)
    # because the physical 3D geometry rarely stretches all the way to the 
    # logical boundaries of the padded 2D collision map canvas.
    tile_size_w = 0.0625
    tile_size_d = 0.0625
    
    return offset_x, offset_z, tile_size_w, tile_size_d


def format_header(scene: str, offset_x, offset_z, tile_size, obj_path, map_path,
                  min_x, max_x, min_z, max_z, tile_cols, tile_rows) -> str:
    guard = f"{scene.upper()}_OFFSETS_H"
    lines = [
        f"// Auto-generated by obj2offsets.py — do not edit by hand",
        f"// OBJ:  {os.path.basename(obj_path)}",
        f"// MAP:  {os.path.basename(map_path) if map_path else 'N/A (tile count entered manually)'}",
        f"//",
        f"// Vertex bounds:  X [{min_x:.6f} → {max_x:.6f}]  Z [{min_z:.6f} → {max_z:.6f}]",
        f"// World size:     {max_x - min_x:.6f} × {max_z - min_z:.6f}",
        f"// Tile grid:      {tile_cols} × {tile_rows}",
        f"",
        f"#ifndef {guard}",
        f"#define {guard}",
        f"",
        f"#define TILE_SIZE      {tile_size:.6f}f  // Fixed scale: 1 unit = 16 tiles",
        f"#define WORLD_OFFSET_X {offset_x:.6f}f  // -minX",
        f"#define WORLD_OFFSET_Z {offset_z:.6f}f  // -minZ",
        f"",
        f"// Debug print — paste into your game loop to verify:",
        f'// iprintf("\\x1b[21;0Htile(x,z): %d, %d",',
        f'//     (int)((charPos.x + WORLD_OFFSET_X) / TILE_SIZE),',
        f'//     (int)((charPos.z + WORLD_OFFSET_Z) / TILE_SIZE));',
        f"",
        f"#endif // {guard}",
    ]
    return "\n".join(lines) + "\n"


# ── CLI ────────────────────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(
        description="Auto-calculate DS collision constants (TILE_SIZE, WORLD_OFFSET_X/Z) "
                    "from a .obj file and its collision map PNG."
    )
    ap.add_argument("obj",        help="Input .obj file")
    ap.add_argument("--map", "-m", default=None,
                    help="Collision map PNG (auto-detected if omitted)")
    ap.add_argument("--tiles", "-t", nargs=2, type=int, metavar=("COLS", "ROWS"),
                    default=None,
                    help="Tile grid size (cols rows) — use instead of --map")
    ap.add_argument("--output", "-o", default=None,
                    help="Output .h file (default: print to stdout)")
    args = ap.parse_args()

    if not os.path.isfile(args.obj):
        print(f"ERROR: OBJ not found: '{args.obj}'", file=sys.stderr)
        sys.exit(1)

    # ── Determine tile count ───────────────────────────────────────────────────
    map_path   = None
    tile_cols  = None
    tile_rows  = None

    if args.tiles:
        tile_cols, tile_rows = args.tiles
        print(f"  Using manual tile grid: {tile_cols}×{tile_rows}")
    else:
        if args.map:
            map_path = args.map
        else:
            map_path = find_map_for_obj(args.obj)
            if map_path:
                print(f"  Auto-detected map: {map_path}")

        if map_path:
            if not os.path.isfile(map_path):
                print(f"ERROR: Map PNG not found: '{map_path}'", file=sys.stderr)
                sys.exit(1)
            tile_cols, tile_rows = get_png_size(map_path)
            print(f"  Map size: {tile_cols}×{tile_rows} tiles")
        else:
            print("  No map PNG found and --map / --tiles not specified.")
            try:
                tile_cols = int(input("  Enter tile column count (map width in pixels): ").strip())
                tile_rows = int(input("  Enter tile row count   (map height in pixels): ").strip())
            except (ValueError, EOFError):
                print("ERROR: Invalid tile count.", file=sys.stderr)
                sys.exit(1)

    # ── Read OBJ bounds ───────────────────────────────────────────────────────
    min_x, max_x, min_z, max_z = get_obj_bounds(args.obj)
    print(f"  OBJ bounds: X [{min_x:.4f} → {max_x:.4f}]  Z [{min_z:.4f} → {max_z:.4f}]")

    # ── Compute ────────────────────────────────────────────────────────────────
    offset_x, offset_z, tile_size_w, tile_size_d = compute_offsets(
        min_x, max_x, min_z, max_z, tile_cols, tile_rows
    )

    if abs(tile_size_w - tile_size_d) > 1e-6:
        print(
            f"  WARNING: Non-square tiles — X tile size {tile_size_w:.6f} "
            f"≠ Z tile size {tile_size_d:.6f}.\n"
            f"           Using X axis (width) for TILE_SIZE. "
            f"Check your collision map aspect ratio.",
            file=sys.stderr,
        )

    tile_size = tile_size_w  # canonical value

    # ── Scene name ────────────────────────────────────────────────────────────
    import re
    stem  = os.path.splitext(os.path.basename(args.obj))[0]
    scene = re.sub(r"[^A-Za-z0-9_]", "_", re.sub(r"_[0-9]+x[0-9]+$", "", stem)).lower()

    # ── Output ────────────────────────────────────────────────────────────────
    header = format_header(
        scene, offset_x, offset_z, tile_size,
        args.obj, map_path,
        min_x, max_x, min_z, max_z,
        tile_cols, tile_rows,
    )

    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(header)
        print(f"  Written: {args.output}")
    else:
        print()
        print(header)


if __name__ == "__main__":
    main()