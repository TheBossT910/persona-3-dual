# Persona 3 Dual — Asset Pipeline

> Everything in `assets/` is source. Everything in `source/` or `nitrofiles/` is generated.  
> **Never hand-edit generated files** — run `make assets` instead.

---

## Quick Start

```bash
# First time only — set up venv
python3 -m venv ~/.venv
source ~/.venv/bin/activate
pip install Pillow

# Full build (assets + ROM)
make

# Asset conversion only
make assets

# One category at a time
make dialogue
make music
make video
make models
make maps
make offsets

# See all targets and flags
make help
```

---

## Directory Layout

```
assets/
  dialogue/       .dlg scripts          → source/dialogue/
  music/          .mp3 music            → nitrofiles/music/
  sfx/            .wav SFX              → soundbank.bin (via mmutil)
  video/          .mp4 cutscenes        → nitrofiles/video/
  models/         .obj 3D models        → assets/models/bin/
  maps/           .png collision maps   → source/maps/

source/
  dialogue/       *_dialogue.h + .cpp   (auto-generated)
  maps/           *.h collision arrays  (auto-generated)
  maps/           *_offsets.h           (auto-generated TILE_SIZE defines)

nitrofiles/
  music/          *.pcm                 (auto-generated)
  video/          *.vid                 (auto-generated)

tools/
  dlg2dialogue.py
  obj2dl.py
  obj2offsets.py
  texture2collision.py
  video2vid.py
```

---

## Tools Reference

---

### `dlg2dialogue.py` — Dialogue compiler

Compiles a `.dlg` scene script into a C++ header + source pair for `DialogueController`.

| | |
|---|---|
| **Input** | `assets/dialogue/<name>.dlg` |
| **Output** | `source/dialogue/<name>_dialogue.h` + `.cpp` |

**Make flag**

```makefile
DLG_FLAGS='--stdout'   # dump to stdout instead of writing files (debug)
```

**Naming** — no special encoding needed. Just use the scene name:

```
assets/dialogue/dormitory.dlg   →   source/dialogue/dormitory_dialogue.h
assets/dialogue/tartarus.dlg    →   source/dialogue/tartarus_dialogue.h
```

---

### `ffmpeg` — Music converter (PCM)

Converts MP3s to raw 16-bit stereo PCM at 32 kHz for `maxmod`.

| | |
|---|---|
| **Input** | `assets/music/<name>.mp3` |
| **Output** | `nitrofiles/music/<name>.pcm` |

No special naming needed. Filenames carry over 1:1.

---

### `video2vid.py` — Video encoder

Encodes MP4 video to an interleaved `.vid` file.

| | |
|---|---|
| **Input** | `assets/video/<name>.mp4` |
| **Output** | `nitrofiles/video/<name>.vid` |

**Make flags**

| Flag | Default | Values |
|---|---|---|
| `VIDEO_BITS` | `8` | `8` (8-bpp palette) or `16` (BGR555) |
| `VIDEO_FPS`  | `15` | any integer |
| `VIDEO_SIZE` | `256x192` | `WxH` string |

```bash
make video VIDEO_FPS=24 VIDEO_BITS=16 VIDEO_SIZE=256x192
```

---

### `obj2dl.py` — 3D model converter

Converts a Wavefront `.obj` into a binary NDS display-list `.bin`.

| | |
|---|---|
| **Input** | `assets/models/<name>[_WxH].obj` |
| **Output** | `assets/models/<name>.bin` |

**Filename encoding — texture size**

Encode the texture dimensions directly in the filename using `_WxH`:

```
dorm.obj            →  --texsize 256 256  (uses MODEL_TEXSIZE default)
dorm_128x128.obj    →  --texsize 128 128
pillar_64x64.obj    →  --texsize 64 64
skybox_512x256.obj  →  --texsize 512 256
```

**Make flag — global fallback**

```bash
make models MODEL_TEXSIZE='128 128'   # default when no _WxH in filename
```

**Optional vertex color**

Not in the Makefile rule (uncommon). Call manually if you need it:

```bash
python3 tools/obj2dl.py assets/models/floor.obj assets/models/floor.bin \
  --texsize 256 256 --color 255 128 0
```

---

### `texture2collision.py` — Collision map converter

Converts a PNG into a `uint8_t collision_map[H][W]` C header.

| | |
|---|---|
| **Input** | `assets/maps/<name>[_X_Y_W_H].png` |
| **Output** | `source/maps/<name>.h` |

**Filename encoding — crop region**

Encode the crop rectangle `(x, y, width, height)` as four underscore-separated integers **at the end** of the filename:

```
lobby.png              →  full image, no crop
lobby_0_0_64_64.png    →  crop x=0, y=0, w=64, h=64
tartarus_32_0_64_64.png→  crop x=32, y=0, w=64, h=64
```

**Make flag**

```bash
MAP_FLAGS='...'   # any extra flags passed to texture2collision.py
```

**Tile values** (edit `TILE_COLORS` in `texture2collision.py`):

| Value | Meaning | Default color |
|---|---|---|
| `0` | Walkable | *(anything unrecognised)* |
| `1` | Collision wall | RGB(197, 0, 0) |
| `2` | Save point | RGB(197, 194, 0) |
| `3` | Prev scene | RGB(0, 92, 197) |
| `4` | Next scene | RGB(56, 197, 0) |
| `100` | Character spawn | RGB(0, 197, 173) |

---

### `obj2offsets.py` — World offset calculator

Reads vertex bounds from a `.obj` and pixel dimensions from its paired collision map PNG, then emits the three C defines you need.

| | |
|---|---|
| **Input** | `assets/models/<name>.obj`  +  `assets/maps/<name>.png` (auto-detected) |
| **Output** | `source/maps/<name>_offsets.h` |

**Auto-detection** — the tool looks for a PNG in `assets/maps/` whose base name matches the `.obj`. You don't need to specify it unless the names differ.

**The generated header looks like:**

```c
// Auto-generated by obj2offsets.py — do not edit by hand
// OBJ:  dorm.obj
// MAP:  dorm.png
//
// Vertex bounds:  X [-2.000000 → 2.000000]  Z [-2.000000 → 2.000000]
// World size:     4.000000 × 4.000000
// Tile grid:      64 × 64

#ifndef DORM_OFFSETS_H
#define DORM_OFFSETS_H

#define TILE_SIZE      0.062500f  // world_width / tile_cols
#define WORLD_OFFSET_X 2.000000f  // -minX
#define WORLD_OFFSET_Z 2.000000f  // -minZ

#endif
```

**Include it in your scene's `.cpp`:**

```cpp
#include "maps/dorm_offsets.h"

// Then use directly:
int tileX = (int)((charPos.x + WORLD_OFFSET_X) / TILE_SIZE);
int tileZ = (int)((charPos.z + WORLD_OFFSET_Z) / TILE_SIZE);
```

**`make offsets`** runs this automatically for every `.obj` in `assets/models/`.

**Manual usage** (when names differ or you want to specify the map explicitly):

```bash
# Auto-detect map
python3 tools/obj2offsets.py assets/models/dorm.obj -o source/maps/dorm_offsets.h

# Explicit map
python3 tools/obj2offsets.py assets/models/dorm.obj \
  --map assets/maps/dorm_0_0_64_64.png \
  -o source/maps/dorm_offsets.h

# Manual tile count (no map PNG)
python3 tools/obj2offsets.py assets/models/dorm.obj --tiles 64 64

# Print to stdout (no file written)
python3 tools/obj2offsets.py assets/models/dorm.obj
```

---

## Naming Convention Summary

| Asset | Filename pattern | Encoded info |
|---|---|---|
| Dialogue | `<scene>.dlg` | — |
| Music | `<name>.mp3` | — |
| SFX | `<name>.mp3` | — |
| Video | `<name>.mp4` | — |
| Model | `<name>_<W>x<H>.obj` | tex size (optional, default = `MODEL_TEXSIZE`) |
| Collision map | `<name>_<x>_<y>_<w>_<h>.png` | crop rect (optional, default = full image) |

---

## Incremental Builds

Make tracks input → output dependencies. Re-running `make assets` only
reconverts files whose source is **newer** than the output. To force a
full reconvert:

```bash
make clean && make assets
```

To force a single category:

```bash
touch assets/video/intro.mp4 && make video
```

---

## Verifying Collision In-Game

After `make offsets`, include the generated header and add this debug block:

```c
#include "maps/dorm_offsets.h"

// In your game loop:
iprintf("\x1b[21;0Htile(x,z): %d, %d",
    (int)((charPos.x + WORLD_OFFSET_X) / TILE_SIZE),
    (int)((charPos.z + WORLD_OFFSET_Z) / TILE_SIZE));
iprintf("\x1b[22;0Hpos(x,z):  %d, %d",
    (int)(charPos.x * 100),
    (int)(charPos.z * 100));
```

Walking from one edge of the world to the other should show tile indices
going from `0` to `(tile count − 1)`. If they're inverted, edit the
`ROTATE_180` in `texture2collision.py`.
