## obj2dl
obj2dl.py *input* *output*
- Converts a .obj file to display list commands
*This tool was AI generated*

## obj2bin
obj2dl.py *input* *output* --texsize *w* *h*
- Converts a .obj file to .bin
*This tool was AI generated*

python3 /Users/taharashid/Desktop/obj2bin.py /Users/taharashid/Desktop/Personal/Coding/nds_dev/models/iwatodai_dorm/iwatodai_dorm.obj uv.bin --texsize 256 256

## convert_map
convert_map.py *input* *output* [x y width height]
- Converts a Blockbench texture PNG to a DS collision map C header file
- Each pixel in the texture corresponds to one tile
- Optionally crop a region of the texture using x, y, width, height
*This tool was AI generated*

python3 convert_map.py /Users/taharashid/Desktop/texture.png output.h 0 0 64 64

---
Activate .venv

python3 -m venv ~/.venv
source ~/.venv/bin/activate
---

## Figuring Out World Offset and Tile Size

When setting up collision, you need three values that describe how your .obj world maps to your tile grid:
- `WORLD_OFFSET_X` / `WORLD_OFFSET_Z` — shifts the world so the minimum corner is at 0
- `TILE_SIZE` — how many world units wide one tile is

### Step 1 — Get the exact vertex bounds of your .obj

Run this in terminal:

```bash
grep "^v " environment.obj | awk '{
    if (NR==1) { minX=$2; maxX=$2; minZ=$4; maxZ=$4 }
    if ($2 < minX) minX=$2; if ($2 > maxX) maxX=$2;
    if ($4 < minZ) minZ=$4; if ($4 > maxZ) maxZ=$4;
} END { print "X:", minX, "to", maxX, "| Z:", minZ, "to", maxZ }'
```

Example output: `X: -2.0 to 2.0 | Z: -2.0 to 2.0`

### Step 2 — Calculate the values

```
world width  = maxX - minX        (e.g. 2.0 - (-2.0) = 4.0)
world depth  = maxZ - minZ        (e.g. 2.0 - (-2.0) = 4.0)

WORLD_OFFSET_X = -minX            (e.g. -(-2.0) = 2.0)
WORLD_OFFSET_Z = -minZ            (e.g. -(-2.0) = 2.0)

TILE_SIZE = world width / tile count   (e.g. 4.0 / 64 = 0.0625)
```

The tile count is the pixel resolution of your painted collision map (e.g. 64 if you painted a 64x64 region).

### Step 3 — Paste into your C code

```c
#define TILE_SIZE      0.0625f   // world width / tile count
#define WORLD_OFFSET_X 2.0f     // -minX
#define WORLD_OFFSET_Z 2.0f     // -minZ
```

### Step 4 — Verify in game

Add this debug print and walk around:

```c
iprintf("\x1b[11;0Hpos: %.2f, %.2f   ", translateX, translateZ);
iprintf("\x1b[12;0Htile: %d, %d      ",
    (int)((translateX + WORLD_OFFSET_X) / TILE_SIZE),
    (int)((translateZ + WORLD_OFFSET_Z) / TILE_SIZE));
```

Walking from one edge of the world to the other should show tiles going from `0` to `(tile count - 1)`. If the numbers are inverted or offset, the collision map orientation may need adjusting in convert_map.py (see flip/rotate options).

## Compress MP3 to DS format
ffmpeg -i input.mp3 -ar 22050 -ac 1 -q:a 4 output.mp3