## Activate .venv

python3 -m venv ~/.venv
source ~/.venv/bin/activate

---

## Convert .dlg to dialogue code
python3 dlg2dialogue.py input.dlg

---

## Convert MP3 to .pcm (NDS format)
ffmpeg -i input.mp3 -f s16le -ar 32000 -ac 2 output.pcm

---

# video2vid - Convert videos to .vid (NDS format) - 
python3 video2vid.py input.mp4 output --bits 8 --fps 15
*This tool was AI generated*

---

## obj2bin - Convert 3D models into .bin display list data (NDS format)
obj2dl.py *input* *output* --texsize *w* *h*
- Converts a .obj file to .bin
*This tool was AI generated*

python3 /Users/taharashid/Desktop/obj2bin.py /Users/taharashid/Desktop/Personal/Coding/nds_dev/models/iwatodai_dorm/iwatodai_dorm.obj uv.bin --texsize 256 256

## convert_map - - Convert a 2D collision texture into .h collision data (NDS format)
texture2collision.py *input* *output* [x y width height]
- Converts a Blockbench texture PNG to a DS collision map C header file
- Each pixel in the texture corresponds to one tile
- Optionally crop a region of the texture using x, y, width, height
*This tool was AI generated*

python3 texture2collision.py /Users/taharashid/Desktop/texture.png output.h 0 0 64 64

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
// print coordinates (64x64 area from 0,0 to 64,64)
    iprintf("\x1b[21;0Htile(x,z): %d, %d",
        (int)((charPos.x + worldOffsetX) / tileSize),
        (int)((charPos.z + worldOffsetZ) / tileSize));
    iprintf("\x1b[22;0Htranslate(x,z): %d, %d",
        (int)(charPos.x * 100),
        (int)(charPos.z * 100));
    iprintf("\x1b[23;0Hangle(w,c): %d, %d", (int)(charPos.angle * 100), (int)(charPos.facingAngle * 100));
```

Walking from one edge of the world to the other should show tiles going from `0` to `(tile count - 1)`. If the numbers are inverted or offset, the collision map orientation may need adjusting in convert_map.py (see flip/rotate options).