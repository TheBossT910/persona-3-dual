#!/usr/bin/env python3
"""
nds_interweave.py — Nintendo DS video + audio encoder
Encodes video and audio, then interweaves them into a single .vid file 
to prevent SD card seeking delays.
"""

import argparse
import os
import struct
import subprocess
import sys
import tempfile

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
    cmd = [
        "ffmpeg", "-y", "-i", input_path, "-an",
        "-vcodec", "rawvideo", "-f", "rawvideo",
        "-pix_fmt", "bgr555le", "-s", size, "-r", str(fps), out_raw,
    ]
    subprocess.run(cmd, stderr=subprocess.PIPE, text=True, check=True)

def patch_alpha_bits(src: str, dst: str):
    with open(src, "rb") as f:
        data = bytearray(f.read())
    for i in range(1, len(data), 2):
        data[i] |= 0x80
    with open(dst, "wb") as f:
        f.write(data)

def encode_8bit_raw(input_path: str, fps: int, size: str, output_raw: str, output_pal: str):
    w, h = (int(x) for x in size.split("x"))
    with tempfile.TemporaryDirectory() as tmp_dir:
        palette_png = os.path.join(tmp_dir, "palette.png")
        frames_tmp = os.path.join(tmp_dir, "frames.tmp")

        subprocess.run([
            "ffmpeg", "-y", "-i", input_path,
            "-vf", f"scale={w}:{h},palettegen=max_colors=256:stats_mode=full",
            "-frames:v", "1", palette_png,
        ], stderr=subprocess.PIPE, text=True, check=True)

        subprocess.run([
            "ffmpeg", "-y", "-i", input_path, "-i", palette_png,
            "-lavfi", f"scale={w}:{h} [x]; [x][1:v] paletteuse=dither=bayer",
            "-an", "-vcodec", "rawvideo", "-f", "rawvideo",
            "-pix_fmt", "pal8", "-r", str(fps), frames_tmp,
        ], stderr=subprocess.PIPE, text=True, check=True)

        from PIL import Image
        pal_img = Image.open(palette_png).convert("RGB")
        pal_data = list(pal_img.getdata())
        bgr555 = bytearray(512)
        for i, (r8, g8, b8) in enumerate(pal_data[:256]):
            r5, g5, b5 = r8 >> 3, g8 >> 3, b8 >> 3
            word = r5 | (g5 << 5) | (b5 << 10)
            struct.pack_into("<H", bgr555, i * 2, word)

        with open(output_pal, "wb") as f:
            f.write(bgr555)

        with open(frames_tmp, "rb") as src, open(output_raw, "wb") as dst:
            while True:
                frame_data = src.read(w * h)
                if not frame_data: break
                dst.write(frame_data)
                src.read(1024) # Strip ffmpeg palette

def extract_pcm(input_path: str, out_pcm: str):
    cmd = [
        "ffmpeg", "-y", "-i", input_path,
        "-f", "s16le", "-ar", "32000", "-ac", "2", out_pcm
    ]
    subprocess.run(cmd, stderr=subprocess.PIPE, text=True, check=True)

def interweave(raw_video: str, pcm_audio: str, out_vid: str, fps: int, w: int, h: int, bpp: int, pal_file: str = None):
    frame_size = w * h * bpp
    sample_rate = 32000
    bytes_per_sample = 4 # 16-bit stereo = 4 bytes per sample

    with open(raw_video, "rb") as f_vid, open(pcm_audio, "rb") as f_aud, open(out_vid, "wb") as f_out:
        if pal_file:
            with open(pal_file, "rb") as f_pal:
                f_out.write(f_pal.read(512))

        frame_idx = 0
        while True:
            v_data = f_vid.read(frame_size)
            if not v_data: break

            # Calculate exact audio samples needed to prevent drift over time
            start_sample = int(frame_idx * sample_rate / fps)
            end_sample = int((frame_idx + 1) * sample_rate / fps)
            samples_this_frame = end_sample - start_sample
            audio_bytes = samples_this_frame * bytes_per_sample
            
            a_data = f_aud.read(audio_bytes)
            
            # If audio ends before video, pad with silence
            if len(a_data) < audio_bytes:
                a_data += b'\x00' * (audio_bytes - len(a_data))

            # Write: [u32 audio_size] [audio_data] [video_data]
            f_out.write(struct.pack("<I", audio_bytes))
            f_out.write(a_data)
            f_out.write(v_data)

            frame_idx += 1
    return frame_idx

def main():
    parser = argparse.ArgumentParser(description="Interleave video/audio for DS.")
    parser.add_argument("input", help="Source video")
    parser.add_argument("output", help="Output base name")
    parser.add_argument("--bits", type=int, choices=[8, 16], default=16)
    parser.add_argument("--fps", type=int, default=24)
    parser.add_argument("--size", default="256x192")
    args = parser.parse_args()

    check_ffmpeg()
    w, h = (int(x) for x in args.size.split("x"))
    bpp = 2 if args.bits == 16 else 1
    output_file = args.output + ".vid"

    print("Processing...")
    with tempfile.TemporaryDirectory() as tmp_dir:
        raw_vid = os.path.join(tmp_dir, "v.raw")
        pcm_aud = os.path.join(tmp_dir, "a.pcm")
        pal_file = os.path.join(tmp_dir, "p.pal") if args.bits == 8 else None

        extract_pcm(args.input, pcm_aud)

        if args.bits == 16:
            tmp_vid = os.path.join(tmp_dir, "tmp.raw")
            run_ffmpeg_16bit(args.input, tmp_vid, args.fps, args.size)
            patch_alpha_bits(tmp_vid, raw_vid)
        else:
            encode_8bit_raw(args.input, args.fps, args.size, raw_vid, pal_file)

        frames = interweave(raw_vid, pcm_aud, output_file, args.fps, w, h, bpp, pal_file)

    print(f"Done! Created {output_file} ({frames} frames)")

if __name__ == "__main__":
    main()