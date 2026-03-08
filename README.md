# persona-3-ds

A Nintendo DS demake of **Persona 3**, developed in C++ using devkitpro. This project is a personal deep-dive into embedded systems and low-level graphics programming, building a game engine from scratch without external game libraries.

This project began as a way to explore more embedded programming in a fun way. I always wanted to make a game, so I thought why not use embedded to make a game? I decided on developing a game for the Nintendo DS as it was my first ever game console, and I grew up with it. It is a portable, sleek, and popular game console with a large homebrew and hacking community and supports basic 3D, which is what I wanted. Plus, it has some unique hardware like dual screens, a touch screen, and microphone to use if I ever want to (which I plan to!).

I chose to make a demake of one of my favourite games, Persona 3, for a few reasons. I really enjoyed the game and there wasn't an official DS version (or any custom version for the DS that I found). Also, Persona has a lot of 2D graphics which would make game development easier in my opinion. And I'm not awefully creative, so basing my game on a pre-existing game would allow me to really focus on coding while also giving me a good story, basic gameplay mechanics, and assets to use and base my own custom assets off of.

> **Status:** Active development: basic intro sequence and main menu complete, 3D environment (Iwatodai Dorm) in progress.

---

## Screenshots

<img width="444" height="664" alt="title" src="https://github.com/user-attachments/assets/30f28b79-a37f-48cc-baad-4a9dc6261b18" />
<img width="441" height="662" alt="menu" src="https://github.com/user-attachments/assets/936abc86-dee8-445b-8e11-7ca73f57900e" />
<img width="442" height="658" alt="dorm" src="https://github.com/user-attachments/assets/396a87fa-0cf7-445d-8d2a-e6cec6f9173a" />


---

## Features

- **Intro & Main Menu** — Animated intro sequence with multi-layer 2D compositing, sinusoidal sprite/background animations, and fade effects
- **State Machine Architecture** — Modular view-based state machine with clean separation between scenes (Intro, Main Menu, Iwatodai Dorm)
- **2D Graphics Pipeline** — Manual VRAM bank allocation supporting 4 simultaneous background layers with extended palette support
- **3D Environment** — OpenGL-like fixed-function 3D rendering with display list geometry, UV-mapped textures, and free camera controls (rotate + translate)
- **Custom Tooling** — Python OBJ-to-display-list converter (`obj2bin.py`) for importing Blockbench/Blender models into the DS's 3D engine
- **Hand-modelled Assets** — 5+ low-poly environment models (desk, chairs, wardrobe, lamps, TV, doors) built in Blockbench

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
- [ ] Character model + movement controls
- [ ] Collision + interaction detection
- [ ] Iwatodai Dorm — fully detailed environment
- [ ] Additional scenes (world map, school room, Tartarus)
- [ ] Music/SFX playback
- [ ] Interaction system

---

## Inspiration

Based on **Persona 3 FES** (PS2) and **Persona 3 Reload** (Steam). This is a fan project for educational purposes and is not affiliated with or endorsed by Atlus or Sega.
