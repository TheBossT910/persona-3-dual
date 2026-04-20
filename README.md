# Persona 3 Dual

A Nintendo DS demake of **Persona 3**, developed in C++ using devkitpro. This project is a personal deep-dive into embedded systems and low-level graphics programming, building a game engine from scratch without external game libraries.

This project began as a way to explore more embedded programming in a fun way. I always wanted to make a game, so I thought why not use embedded to make a game? I decided on developing a game for the Nintendo DS as it was my first ever game console, and I grew up with it. It is a portable, sleek, and popular game console with a large homebrew and hacking community and supports basic 3D, which is what I wanted. Plus, it has some unique hardware like dual screens, a touch screen, and microphone to use if I ever want to (which I plan to!).

I chose to make a demake of one of my favourite games, Persona 3, for a few reasons. I really enjoyed the game and there wasn't an official DS version (or any custom version for the DS that I found). Also, Persona has a lot of 2D graphics which would make game development easier in my opinion. And I'm not awefully creative, so basing my game on a pre-existing game would allow me to really focus on coding while also giving me a good story, basic gameplay mechanics, and assets to use and base my own custom assets off of.

Later on (around April 2026), I found out that there was an online joke about having a DS version of Persona 3 called "Persona 3 Dual". I ended up renaming my project from "Persona 3 DS" to "Persona 3 Dual" as a fun nod to this joke (and to make it into reality!)

> **Status:** Active development: basic intro sequence and main menu complete, 3D environment (Iwatodai Dorm) in progress.

<img width="444" height="519" alt="20260417_231005" src="https://github.com/user-attachments/assets/8dd225c8-f3eb-4b80-8ec4-feb381e4b71d" />

---

## Screenshots
Home
<img width="395" height="588" alt="Screenshot 2026-04-17 at 11 17 18 PM" src="https://github.com/user-attachments/assets/735128dd-a5aa-403f-8d94-748d6294e27c" />

Menu
<img width="394" height="584" alt="Screenshot 2026-04-17 at 11 17 35 PM" src="https://github.com/user-attachments/assets/72eb8264-9e56-4666-84a9-07bfe96a0a78" />

Iwatodai Dorm (WIP)
<img width="395" height="583" alt="Screenshot 2026-04-17 at 11 17 58 PM" src="https://github.com/user-attachments/assets/dad1562b-67ae-4566-9d04-f2a9966ba916" />

---

## Features

- **Animated Intro & Main Menu** — Multi-layer 2D compositing with sinusoidal sprite/background animations, fade effects, and a custom logo split across two stitched sprites. The sub-screen has a static layer plus an animated attributions layer. "Press Start" animates independently of screen brightness.
- **State Machine Architecture** — View-based state machine with clean scene separation (Intro → Main Menu → Iwatodai Dorm). Each scene has its own `.cpp`/`.h` pair with dedicated init and cleanup functions.
- **2D Graphics Pipeline** — Manual VRAM bank allocation supporting 4 simultaneous background layers with extended palette support. VRAM placement is calculated via tile-count analysis to prevent memory corruption.
- **3D Environment** — Fixed-function 3D rendering with display list geometry, UV-mapped textures, and free camera controls (rotate + translate). The camera orbits and follows the player, updating movement direction relative to facing angle.
- **Collision & Interaction System** — Tile-based 2D collision map overlaid on the 3D world. Zones are typed (wall, save point, scene transition, NPC trigger) to drive interactivity like zone loading and character encounters.
- **Dialogue Controller** — Fully-featured dialogue system with character-by-character text animation, per-line character portraits, branching dialogue trees with d-pad navigation, and scrollable history. Dialogue is authored in plain header files.
- **Decoupled Controller Architecture** — Character movement, camera, and dialogue are each in separate files, making the codebase modular and easy to extend.
- **Audio** — MP3 playback via a ported `nds-libmad` library, streaming audio in via NitroFS. Also supports SFX
- **Video** - Video playback via streaming in frames, paired with the audio solution above to stitch together a full video with audio. Uses NitroFS.
- **Custom Tooling** — Python utilities built alongside the engine: `obj2bin.py` converts Wavefront OBJ files (with UV data) to DS display list binaries; a Blockbench texture → header file converter is also included. A Markdown → dialogue header generator is planned.
- **Hand-modelled Assets** — 10+ low-poly environment models built in Blockbench: receptionist desk, dining table, chairs (2 variants), wardrobe, side table, lamps, TV, and doors.

---

## Technical Details

| | |
|---|---|
| **Platform** | Nintendo DS |
| **CPU** | ARM9 (main) / ARM7 (sound/system) |
| **Toolchain** | devkitPro (devkitARM + libnds) |
| **Language** | C++, Python |
| **3D API** | DS fixed-function engine (OpenGL-like) |
| **Emulator** | melonDS |
| **3D Modelling** | Blockbench |

---

## Roadmap

- [x] Intro sequence with animated backgrounds and sprites
- [x] Main menu with interactive option selector
- [x] 3D environment rendering with UV-mapped textures
- [x] Basic camera controls
- [x] Character model + movement controls
- [x] Collision + interaction detection
- [x] Dialogue system
- [x] Music/SFX playback
- [ ] Combat system
- [ ] Iwatodai Dorm — fully detailed environment
- [ ] Additional scenes (world map, school room, Tartarus)

---

## Inspiration

Based on **Persona 3 FES** (PS2) and **Persona 3 Reload** (Steam) games, and inspired by the **Persona 3 Dual** online joke. This is a fan project for educational purposes and is not affiliated with or endorsed by Atlus or Sega.
