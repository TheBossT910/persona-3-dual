#!/usr/bin/env python3
"""
nds_encode.py — Nintendo DS video encoder
Wraps ffmpeg + DS post-processing into a single step.

Usage examples:
  # 16-bit, 24fps (default)
  python nds_encode.py input.mp4 intro

  # 8-bit, 15fps (recommended for HW)
  python nds_encode.py input.mp4 intro --bits 8 --fps 15

  # 16-bit, half resolution
  python nds_encode.py input.mp4 intro --bits 16 --fps 24 --size 128x96
"""

import argparse
import os
import struct
import subprocess
import sys
import tempfile


# ── Helpers ──────────────────────────────────────────────────────────────────

def check_ffmpeg():
    try:
        subprocess.run(
            ["ffmpeg", "-version"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=True,
        )
    except FileNotFoundError:
        print("Error: ffmpeg not found. Install it and make sure it's on your PATH.")
        sys.exit(1)


def run_ffmpeg_16bit(input_path: str, out_raw: str, fps: int, size: str):
    """Single-pass 16-bit bgr555le encode."""
    cmd = [
        "ffmpeg", "-y",
        "-i", input_path,
        "-an",
        "-vcodec", "rawvideo",
        "-f", "rawvideo",
        "-pix_fmt", "bgr555le",
        "-s", size,
        "-r", str(fps),
        out_raw,
    ]
    print(f"Running ffmpeg → bgr555le @ {size} {fps}fps …")
    r = subprocess.run(cmd, stderr=subprocess.PIPE, text=True)
    if r.returncode != 0:
        print("ffmpeg failed:\n", r.stderr)
        sys.exit(1)
    print("ffmpeg finished.\n")


def patch_alpha_bits(src: str, dst: str):
    """
    16-bit only.
    NDS BMP16 needs bit 15 set per pixel for the display engine to treat
    pixels as opaque. ffmpeg bgr555le leaves it clear.
    """
    print("Patching alpha bits …")
    with open(src, "rb") as f:
        data = bytearray(f.read())
    for i in range(1, len(data), 2):
        data[i] |= 0x80
    with open(dst, "wb") as f:
        f.write(data)
    print(f"Patched {len(data) // 2:,} pixels.\n")


def encode_8bit(input_path: str, fps: int, size: str, output_file: str):
    """
    Two-pass 8-bit encode:
      Pass 1 — build an optimal 256-color palette from the full video.
      Pass 2 — quantize every frame against that palette (pal8 raw).
    Output format: [512-byte BGR555 palette] + [raw indexed frame data]
    The C++ side reads the 512-byte header once at init, then streams frames.
    """
    try:
        from PIL import Image
    except ImportError:
        print("Error: Pillow is required for 8-bit encoding.")
        print("  pip install pillow")
        sys.exit(1)

    w, h = (int(x) for x in size.split("x"))

    with tempfile.TemporaryDirectory() as tmp_dir:
        palette_png = os.path.join(tmp_dir, "palette.png")
        frames_raw  = os.path.join(tmp_dir, "frames.raw")

        # ── Pass 1: palettegen ────────────────────────────────────────────────
        print("Pass 1: generating palette …")
        cmd1 = [
            "ffmpeg", "-y", "-i", input_path,
            "-vf", f"scale={w}:{h},palettegen=max_colors=256:stats_mode=full",
            "-frames:v", "1",
            palette_png,
        ]
        r = subprocess.run(cmd1, stderr=subprocess.PIPE, text=True)
        if r.returncode != 0:
            print("ffmpeg palettegen failed:\n", r.stderr)
            sys.exit(1)

        # ── Pass 2: paletteuse → pal8 raw ────────────────────────────────────
        print("Pass 2: quantizing frames …")
        cmd2 = [
            "ffmpeg", "-y",
            "-i", input_path, "-i", palette_png,
            "-lavfi", f"scale={w}:{h} [x]; [x][1:v] paletteuse=dither=bayer",
            "-an",
            "-vcodec", "rawvideo",
            "-f", "rawvideo",
            "-pix_fmt", "pal8",
            "-r", str(fps),
            frames_raw,
        ]
        r = subprocess.run(cmd2, stderr=subprocess.PIPE, text=True)
        if r.returncode != 0:
            print("ffmpeg paletteuse failed:\n", r.stderr)
            sys.exit(1)
        print("ffmpeg finished.\n")

        # ── Convert palette PNG → 512-byte NDS BGR555 blob ───────────────────
        print("Converting palette to NDS BGR555 …")
        pal_img  = Image.open(palette_png).convert("RGB")
        pal_data = list(pal_img.getdata())   # list of (R,G,B) tuples
        bgr555   = bytearray(512)            # 256 colors × 2 bytes, zero-padded
        for i, (r8, g8, b8) in enumerate(pal_data[:256]):
            r5   = r8 >> 3
            g5   = g8 >> 3
            b5   = b8 >> 3
            word = r5 | (g5 << 5) | (b5 << 10)   # BGR555, bit15=0 (opaque in 8bpp)
            struct.pack_into("<H", bgr555, i * 2, word)

        # ── Write final file: palette header + raw pixel data ─────────────────
        print(f"Writing {output_file} …")
        with open(frames_raw, "rb") as src, open(output_file, "wb") as dst:
            dst.write(bgr555)       # 512-byte palette header read once at NDS init
            
            # FIX: Strip the 1024-byte palette FFmpeg embeds inside every frame
            while True:
                frame_data = src.read(w * h)
                if not frame_data:
                    break
                dst.write(frame_data)
                src.read(1024)      # Discard the embedded FFmpeg palette


# ── Main ─────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Encode a video for the Nintendo DS in one step.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("input",  help="Source video file (e.g. input.mp4)")
    parser.add_argument("output", help="Output base name, no extension (e.g. 'intro' → intro.raw)")
    parser.add_argument("--bits", type=int, choices=[8, 16], default=16,
                        help="Colour depth: 16 = bgr555le (default), 8 = indexed palette")
    parser.add_argument("--fps",  type=int, default=24,
                        help="Target framerate (default: 24). 12–15 recommended for HW.")
    parser.add_argument("--size", default="256x192",
                        help="Resolution WxH (default: 256x192).")
    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"Error: '{args.input}' not found.")
        sys.exit(1)

    output_file = args.output if args.output.endswith(".raw") else args.output + ".raw"
    check_ffmpeg()

    # ── Encode ───────────────────────────────────────────────────────────────
    if args.bits == 16:
        with tempfile.NamedTemporaryFile(suffix=".raw", delete=False) as tmp:
            tmp_path = tmp.name
        try:
            run_ffmpeg_16bit(args.input, tmp_path, args.fps, args.size)
            patch_alpha_bits(tmp_path, output_file)
        finally:
            if os.path.exists(tmp_path):
                os.remove(tmp_path)
    else:
        encode_8bit(args.input, args.fps, args.size, output_file)

    # ── Summary ───────────────────────────────────────────────────────────────
    size_bytes   = os.path.getsize(output_file)
    w, h         = (int(x) for x in args.size.split("x"))
    bytes_per_px = 2 if args.bits == 16 else 1
    frame_size   = w * h * bytes_per_px
    # 8-bit files have a 512-byte palette header before the frame data
    payload      = size_bytes - (512 if args.bits == 8 else 0)
    num_frames   = payload // frame_size
    duration_s   = num_frames / args.fps
    bw           = frame_size * args.fps / 1024

    print(f"\nDone!  →  {output_file}")
    print(f"  Bit depth  : {args.bits}-bit")
    print(f"  Resolution : {args.size}")
    print(f"  FPS        : {args.fps}")
    print(f"  Frames     : {num_frames:,}")
    print(f"  Duration   : {duration_s:.1f}s")
    print(f"  File size  : {size_bytes / 1024:.1f} KB")
    print(f"  Frame size : {frame_size / 1024:.1f} KB")
    print(f"  Bandwidth  : {bw:.0f} KB/s  (NDS cart reads ~300–400 KB/s)")
    if bw > 350:
        print("  ⚠  Bandwidth may exceed cart read speed — consider lower --fps or --size.")


if __name__ == "__main__":
    main()