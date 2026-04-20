#!/usr/bin/env python3
import argparse
import sys
import os

def process_video(input_file, output_file):
    # Check if the input file actually exists
    if not os.path.exists(input_file):
        print(f"❌ Error: Could not find '{input_file}'")
        sys.exit(1)

    print(f"📂 Reading '{input_file}'...")
    with open(input_file, "rb") as f:
        data = bytearray(f.read())

    print("⚙️  Setting alpha bits for Nintendo DS...")
    
    # The DS expects little-endian 16-bit integers.
    # We set the highest bit (0x80) of every second byte to 1.
    for i in range(1, len(data), 2):
        data[i] |= 0x80 

    print(f"💾 Saving to '{output_file}'...")
    with open(output_file, "wb") as f:
        f.write(data)

    print("✅ Done! File is ready for your libnds project.")

def main():
    # Setup the terminal argument parser
    parser = argparse.ArgumentParser(
        description="Prepares a raw 16-bit video file for the Nintendo DS by setting the alpha bits."
    )
    parser.add_argument("input", help="The raw video file from ffmpeg (e.g., video_24fps.raw)")
    parser.add_argument("output", help="The name of the DS-ready file to save (e.g., intro_ds.raw)")

    args = parser.parse_args()
    
    process_video(args.input, args.output)

if __name__ == "__main__":
    main()