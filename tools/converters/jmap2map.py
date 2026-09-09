#!/usr/bin/env python3
"""
jmap2map.py - Convert a .jmap collision map to a C header file.

As a module (via build_asset.py):
    Dispatched automatically for .jmap files.

Standalone usage:
    python3 jmap2map.py <input.jmap> <output.h>

See tile_map.json for mapping values.
"""

import json
import os
import re
import argparse
import subprocess
import shutil
import sys
from pathlib import Path


def _load_tile_map():
    """Load TILE_MAP from tile_map.json, searching relative to this script."""
    script_dir = Path(__file__).resolve().parent
    # Check next to this script, then one level up (for converters/ subdir layout)
    candidates = [
        script_dir / "tile_map.json",
        script_dir / "../tile_map.json",
    ]
    for json_path in candidates:
        if json_path.exists():
            with json_path.open("r") as f:
                data = json.load(f)

            raw_map = data.get("TILE_MAP", {})
            clean_map = {}

            # Flatten the dictionary: filter out group strings and merge legacy aliases
            for key, val in raw_map.items():
                if key.startswith("_group_"):
                    continue
                if key == "_legacy_aliases":
                    for leg_k, leg_v in val.items():
                        clean_map[leg_k] = leg_v
                    continue

                clean_map[key] = val

            return clean_map

    raise FileNotFoundError(
        f"Could not find tile_map.json. Searched: {[str(p) for p in candidates]}"
    )


def parse_jmap(path_obj, tile_map):
    rows = []
    with path_obj.open("r") as f:
        for raw in f:
            line = raw.split("#")[0].strip()
            if not line:
                continue
            tokens = [t.strip() for t in line.split(",") if t.strip()]
            if not tokens:
                continue
            row = []
            for tok in tokens:
                if tok not in tile_map:
                    raise ValueError(f"Unknown tile token '{tok}' in {path_obj.name}")
                row.append(tile_map[tok])
            rows.append(row)
    return rows


def validate(rows, path_obj):
    if not rows:
        raise ValueError(f"No map data found in {path_obj.name}")
    width = len(rows[0])
    for i, row in enumerate(rows):
        if len(row) != width:
            raise ValueError(
                f"Row {i} has {len(row)} tiles, expected {width} (in {path_obj.name})"
            )
    return len(rows), width


def to_header(rows, height, width, stem):
    guard = re.sub(r"[^A-Z0-9]", "_", stem.upper()) + "_H"
    define_prefix = re.sub(r"[^A-Z0-9]", "_", stem.upper())

    lines = []
    lines.append(f"// Auto-generated from {stem}.jmap - do not edit by hand")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append("")
    lines.append(f"#define {define_prefix}_MAP_WIDTH  {width}")
    lines.append(f"#define {define_prefix}_MAP_HEIGHT {height}")
    lines.append("")

    # CRITICAL FIX: Changed uint8_t to uint16_t to support tile IDs > 255
    lines.append(
        f"[[maybe_unused]] static uint16_t {stem}_map[{define_prefix}_MAP_HEIGHT][{define_prefix}_MAP_WIDTH] = {{"
    )
    for r, row in enumerate(rows):
        comma = "," if r < height - 1 else ""
        values = ", ".join(str(v) for v in row)
        lines.append(f"    {{ {values} }}{comma}")
    lines.append("};")
    lines.append("")
    lines.append("#endif")
    return "\n".join(lines) + "\n"


# Entry point for build_asset.py dispatch


def _format_cpp_h_files(paths: list[str]) -> None:
    clang_format = shutil.which("clang-format")
    if clang_format is None:
        return
    inputs = [p for p in paths if os.path.exists(p)]
    if not inputs:
        return
    try:
        subprocess.run([clang_format, "--style=file", "-i", *inputs], check=True)
    except subprocess.CalledProcessError:
        print("Warning: clang-format failed on generated files.", file=sys.stderr)


def convert(input_file: str, output_file: str, config: dict) -> None:
    """build_asset.py-compatible entry point."""
    tile_map = _load_tile_map()

    jmap_path = Path(input_file)
    out_path = Path(output_file)

    # Always generate .hpp for this project’s map headers.
    if out_path.is_dir() or (not out_path.suffix):
        out_path = out_path / (jmap_path.stem + ".hpp")
    elif out_path.suffix.lower() in {".h", ".hh", ".hxx"}:
        out_path = out_path.with_suffix(".hpp")

    stem = jmap_path.stem

    rows = parse_jmap(jmap_path, tile_map)
    height, width = validate(rows, jmap_path)
    header = to_header(rows, height, width, stem)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(header)
    _format_cpp_h_files([str(out_path)])

    print(f"Written: {out_path.name} / ({width}x{height})")


# Standalone CLI


def main():
    parser = argparse.ArgumentParser(
        description="Convert a .jmap collision map to a hpp header file."
    )
    parser.add_argument("input", help="Input .jmap file")
    parser.add_argument("output", help="Output .hpp file (or directory)")
    args = parser.parse_args()

    convert(args.input, args.output, {})


if __name__ == "__main__":
    main()
