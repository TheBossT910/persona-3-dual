#!/usr/bin/env python3
"""
dialogue_compiler.py
====================
Compiles a .dlg script into C++ code for DialogueController.

One .dlg file = one scene/view. A scene contains one or more named
[interaction] blocks. Each interaction compiles to its own array,
init function, and first() accessor.

Background system
-----------------
All dialogue backgrounds share ONE hardware BG slot. Each interaction
has its own ordered list of unique bg names. dialogue.imageId stores the
INDEX into that list (0, 1, 2, ...), not a hardware slot handle.

When DialogueController advances to a line, it calls a user-supplied
loader callback with that index. The callback looks up the right tile-
loading function in the interaction's bg_loaders[] table and calls it.

The compiler emits:
  - <scene>_<ia>_bg_names[]    const char* table (for reference/debug)
  - <scene>_<ia>_bg_loaders[]  void(*)() function pointer table, you fill in
  - <scene>_<ia>_load_bg(int)  dispatcher called by DialogueController
  - <scene>_<ia>_load()        stub: bgInit, fill bg_loaders, call _init()
  - <scene>_<ia>_unload()      stub: bgFree / hide

A compile-time WARNING is printed if any interaction uses > 4 distinct bgs.

Usage:
    python dialogue_compiler.py scene.dlg           -> scene_dialogue.h / .cpp
    python dialogue_compiler.py scene.dlg -o park   -> park_dialogue.h / .cpp
    python dialogue_compiler.py scene.dlg --stdout  -> print to stdout
"""

import re
import sys
import os
import argparse
from dataclasses import dataclass, field
from typing import Optional


# Data model
MAX_BG_SLOTS = 4   # DS hardware limit — warn above this

@dataclass
class Selection:
    label: str
    target: str


@dataclass
class DialogueLine:
    index: int
    label: str
    character: str
    text: str
    bg: str             # bg name string e.g. "bgAkihiko"
    bg_index: int = 0   # index into the interaction's ordered bg list
    selections: list[Selection] = field(default_factory=list)
    prev_index: Optional[int] = None
    next_index: Optional[int] = None


@dataclass
class Interaction:
    name: str
    description: str
    lines: list[DialogueLine] = field(default_factory=list)
    label_map: dict[str, int] = field(default_factory=dict)
    bg_order: list[str] = field(default_factory=list)   # ordered unique bg names


# Parser
class ParseError(Exception):
    def __init__(self, line_num: int, msg: str):
        super().__init__(f"Line {line_num}: {msg}")


class DialogueParser:
    def __init__(self, src: str):
        self.src = src
        self.lines_raw = src.splitlines()
        self.pos = 0
        self.interactions: list[Interaction] = []
        self.scene_bg_defaults: dict[str, str] = {}

    def _raw_line(self):
        while self.pos < len(self.lines_raw):
            n = self.pos + 1
            raw = self.lines_raw[self.pos]
            self.pos += 1
            s = raw.strip()
            if not s or s.startswith("#"):
                continue
            return n, s
        return None, None

    def _peek(self):
        saved = self.pos
        n, s = self._raw_line()
        self.pos = saved
        return n, s

    def _parse_interaction(self, interaction: Interaction):
        local_bg: dict[str, str] = {}
        pending_label: Optional[str] = None
        auto_idx = 0
        pending_jumps: list[tuple[int, str]] = []
        dialogue_ends = {}
        counter = 0

        def resolve_bg(char, override):
            if override:
                bg = override
            elif char in local_bg:
                bg = local_bg[char]
            elif char in self.scene_bg_defaults:
                bg = self.scene_bg_defaults[char]
            else:
                safe = re.sub(r"[^A-Za-z0-9_]", "", char)
                bg = f"bg{safe}"
            if bg not in interaction.bg_order:
                interaction.bg_order.append(bg)
            return bg

        def next_auto_label():
            nonlocal auto_idx
            lbl = f"line_{auto_idx}"
            auto_idx += 1
            return lbl
        
        def split_line(text):
            tokens = text.split(" ")
            currentLine = ""
            finalLine = ""
            
            for token in tokens:
                lineSize = len(currentLine) + len(token)
                if (lineSize > 32):                    
                    finalLine = finalLine + currentLine + (32 - len(currentLine))*" "
                    currentLine = token + " "
                else:
                    currentLine = currentLine + token + " "
            
            finalLine = finalLine + currentLine
            
            if (len(finalLine) == 0):
                return text
            return finalLine

        def add_line(char, text, bg):
            nonlocal pending_label
            label = pending_label or next_auto_label()
            pending_label = None
            idx = len(interaction.lines)
            if label in interaction.label_map:
                raise ParseError(0, f"Duplicate label '@{label}' in '{interaction.name}'")
            bg_index = interaction.bg_order.index(bg)
            text = split_line(text)
            dl = DialogueLine(index=idx, label=label, character=char,
                              text=text, bg=bg, bg_index=bg_index)            
            interaction.lines.append(dl)
            interaction.label_map[label] = idx
            return dl

        while True:
            pn, ps = self._peek()
            if ps is None:
                break
            if re.match(r"^\[interaction\s*:", ps, re.IGNORECASE):
                break

            n, s = self._raw_line()

            if re.match(r"^==.*==$", s):
                continue

            if m := re.match(r"^@bg\s+(\S+)\s+(.+)$", s):
                local_bg[m.group(1)] = m.group(2)
                continue

            if m := re.match(r"^@(\w+)$", s):
                pending_label = m.group(1)
                continue

            if s.lower() == "[end]":
                dialogue_ends[counter - 1] = True
                continue

            if m := re.match(r"^\[jump\s+@(\w+)\]$", s, re.IGNORECASE):
                if interaction.lines:
                    pending_jumps.append((len(interaction.lines) - 1, m.group(1)))
                continue

            if s.lower() == "[choice]":
                if not interaction.lines:
                    raise ParseError(n, "[choice] before any dialogue line")
                host = interaction.lines[-1]
                while True:
                    _, ps2 = self._peek()
                    if ps2 is None:
                        break
                    if re.match(r"^\[interaction\s*:", ps2, re.IGNORECASE):
                        break
                    if cm := re.match(r"^>\s*(.+?)\s*->\s*@(\w+)$", ps2):
                        self._raw_line()
                        host.selections.append(Selection(label=cm.group(1), target=cm.group(2)))
                    else:
                        break
                if not host.selections:
                    raise ParseError(n, "[choice] block has no '> Option -> @label' entries")
                host.next_index = None
                continue

            if m := re.match(r"^([A-Za-z][A-Za-z0-9_ ]*)(?:\(([^)]+)\))?:\s*(.+)$", s):
                counter = counter + 1
                char = m.group(1).strip()
                bg = resolve_bg(char, m.group(2))
                add_line(char, m.group(3), bg)
                continue
            
            raise ParseError(n, f"Unrecognised syntax: '{s}'")

        # link prev/next
        lines = interaction.lines
        for i, dl in enumerate(lines):
            if i > 0:
                dl.prev_index = i - 1
            if (dialogue_ends.get(i) == True):
                dl.next_index = None
            elif not dl.selections and dl.next_index is None:
                if i + 1 < len(lines):
                    dl.next_index = i + 1

        for (fi, tgt) in pending_jumps:
            if tgt not in interaction.label_map:
                raise ParseError(0, f"[jump] unknown label '@{tgt}' in '{interaction.name}'")
            lines[fi].next_index = interaction.label_map[tgt]

        for dl in lines:
            for sel in dl.selections:
                if sel.target not in interaction.label_map:
                    raise ParseError(0,
                        f"Choice '{sel.label}' -> unknown '@{sel.target}' in '{interaction.name}'")

    def parse(self) -> list[Interaction]:
        while True:
            n, s = self._raw_line()
            if s is None:
                break
            if re.match(r"^==.*==$", s):
                continue
            if m := re.match(r"^@bg\s+(\S+)\s+(.+)$", s):
                self.scene_bg_defaults[m.group(1)] = m.group(2)
                continue
            if m := re.match(r"^\[interaction\s*:\s*(\w+)(?:\s*\|\s*(.+))?\]$", s, re.IGNORECASE):
                ia = Interaction(name=m.group(1), description=(m.group(2) or "").strip())
                self._parse_interaction(ia)
                self.interactions.append(ia)
                continue
            raise ParseError(n, f"Expected [interaction: name] or @bg directive, got: '{s}'")
        return self.interactions


# Code generator
class CodeGenerator:

    def __init__(self, interactions: list[Interaction], scene_name: str):
        self.interactions = interactions
        self.scene = scene_name

    def _ptr(self, ia: Interaction, idx: Optional[int]) -> str:
        if idx is None:
            return "NULL"
        return f"&{self.scene}_{ia.name}_lines[{idx}]"

    def _esc(self, s: str) -> str:
        return s.replace("\\", "\\\\").replace('"', '\\"')

    def _vp(self, ia: Interaction) -> str:
        return f"{self.scene}_{ia.name}"

    def generate_h(self) -> str:
        out = []
        s = self.scene
        out += [
            "#pragma once",
            "// Auto-generated by dialogue_compiler.py — do not edit by hand",
            f"// Scene: {s}",
            "",
            '#include "controllers/DialogueController.h"',
            "",
            "// ── Shared BG slot ──────────────────────────────────────────────────",
            "// All dialogue bgs are swapped in/out of this one hardware slot.",
            "// Assign this before calling any interaction _load():",
            f"//   {s}_dialogue_bg_slot = bgInit(3, BgType_Text8bpp, BgSize_T_256x256, 0, 1);",
            f"extern int {s}_dialogue_bg_slot;",
            "",
        ]
        
        out.append(f"void  {s}_unload();  // stub: hide/free the slot")
        out.append("")

        for ia in self.interactions:
            vp = self._vp(ia)
            n  = len(ia.lines)
            nb = len(ia.bg_order)
            desc = f" — {ia.description}" if ia.description else ""

            out += [
                f"// ── interaction: {ia.name}{desc}",
                f"//    bgs ({nb}): " + ", ".join(
                    f"[{i}]={bg}" for i, bg in enumerate(ia.bg_order)
                ),
                f"extern const char*  {vp}_bg_names[{nb}];",
                f"extern void        (*{vp}_bg_loaders[{nb}])();",
                f"void  {vp}_load_bg(int bgIndex);",
                f"extern dialogue {vp}_lines[{n}];",
                f"void  {vp}_init();",
                f"void  {vp}_load();    // stub: fill bg_loaders[], call _init()",
                f"inline dialogue* {vp}_first() {{ return &{vp}_lines[0]; }}",
            ]
            for dl in ia.lines:
                if not dl.label.startswith("line_"):
                    out.append(
                        f"inline dialogue* {vp}_{dl.label}() "
                        f"{{ return &{vp}_lines[{dl.index}]; }}"
                    )
            out.append("")

        out += [
            f"// Initialise every interaction's struct arrays (call after all _load()s).",
            f"inline void {s}_init_all() {{",
        ]
        for ia in self.interactions:
            out.append(f"    {self._vp(ia)}_init();")
        out += ["}", ""]

        return "\n".join(out)

    def generate_cpp(self) -> str:
        out = []
        s = self.scene
        out += [
            f'#include <nds.h>',
            f'#include "{s}_dialogue.h"',
            "",
            "// Auto-generated by dialogue_compiler.py — do not edit by hand",
            f"// Scene: {s}",
            "",
            "// ── Shared BG slot ──────────────────────────────────────────────────",
            f"int {s}_dialogue_bg_slot = 0;",
            "",
        ]
        
        out.append("// ── BG imports ──────────────────────────────────────────────────────")
        bgSet = set()
        for ia in self.interactions:  
            for i, bg in enumerate(ia.bg_order):
                bgSet.add(f'#include "{ia.bg_order[i] if ia.bg_order else 'myBg'}.h"')
        out.extend(list(bgSet))
        out.append("")
        
        # unload()
        out += [
            f"void {s}_unload() {{",
            f"    bgHide({s}_dialogue_bg_slot);",
            f"    // TODO: free VRAM / reset loaders if needed.",
            f"}}",
            "",
        ]

        for ia in self.interactions:        
            vp  = self._vp(ia)
            n   = len(ia.lines)
            nb  = len(ia.bg_order)
            desc = f" — {ia.description}" if ia.description else ""

            out += [
                f"// ════════════════════════════════════════════════════════════════",
                f"// interaction: {ia.name}{desc}",
                f"// ════════════════════════════════════════════════════════════════",
                "",
            ]

            # bg name table
            bg_name_entries = ", ".join(f'"{bg}"' for bg in ia.bg_order)
            out.append(f"const char* {vp}_bg_names[{nb}] = {{ {bg_name_entries} }};")

            # bg loader table — nullptrs by default
            out.append(f"void (*{vp}_bg_loaders[{nb}])() = {{")
            for i, bg in enumerate(ia.bg_order):
                out.append(f"    nullptr,  // [{i}] {bg}")
            out.append("};")
            out.append("")

            # dispatcher
            out += [
                f"void {vp}_load_bg(int bgIndex) {{",
                f"    if (bgIndex >= 0 && bgIndex < {nb} && {vp}_bg_loaders[bgIndex])",
                f"        {vp}_bg_loaders[bgIndex]();",
                f"}}",
                "",
            ]

            # _load() stub
            out += [
                f"void {vp}_load() {{",
                f"    // Use {s}_dialogue_bg_slot as the target hardware slot.",
            ]
            
            for i, bg in enumerate(ia.bg_order):
                out += [
                f"    ",
                f"    {vp}_bg_loaders[{i}] = [](){{",
                f"        dmaCopy({ia.bg_order[i] if ia.bg_order else 'myBg'}Tiles,",
                f"            bgGetGfxPtr({s}_dialogue_bg_slot),",
                f"            {ia.bg_order[i] if ia.bg_order else 'myBg'}TilesLen);",
                f"        dmaCopy({ia.bg_order[i] if ia.bg_order else 'myBg'}Map,",
                f"            bgGetMapPtr({s}_dialogue_bg_slot),",
                f"            {ia.bg_order[i] if ia.bg_order else 'myBg'}MapLen);",
                f"        vramSetBankH(VRAM_H_LCD);",
                f"        dmaCopy({ia.bg_order[i] if ia.bg_order else 'myBg'}Pal,",
                f"            &VRAM_H_EXT_PALETTE[0][0],",
                f"            {ia.bg_order[i] if ia.bg_order else 'myBg'}PalLen);",
                f"        vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE);",
                f"        bgShow({s}_dialogue_bg_slot);",
                f"    }};",
            ]
                
            out += [
                f"",
                f"    {vp}_init();",
                f"}}",
                "",
            ]

            # dialogue array
            out.append(f"dialogue {vp}_lines[{n}];")
            out.append("")

            # _init()
            out.append(f"void {vp}_init() {{")

            sel_var_map: dict[tuple[int,int], str] = {}
            any_sel = False
            for dl in ia.lines:
                for si, sel in enumerate(dl.selections):
                    var = f"{vp}_sel_{dl.index}_{si}"
                    sel_var_map[(dl.index, si)] = var
                    target_idx = ia.label_map[sel.target]
                    out.append(
                        f'    dialogueSelection {var} = '
                        f'{{ "{self._esc(sel.label)}", false, '
                        f'{self._ptr(ia, target_idx)} }};'
                    )
                    any_sel = True
            if any_sel:
                out.append("")

            for dl in ia.lines:
                prev_ptr = self._ptr(ia, dl.prev_index)
                next_ptr = self._ptr(ia, dl.next_index)
                if dl.selections:
                    sel_inits = ", ".join(
                        sel_var_map[(dl.index, si)] for si in range(len(dl.selections))
                    )
                    sel_block = f"{{ {sel_inits} }}"
                else:
                    sel_block = "{}"
                label_cmt = f"  // @{dl.label}" if not dl.label.startswith("line_") else ""
                bg_cmt    = f" /* {dl.bg} */"
                out.append(
                    f'    {vp}_lines[{dl.index}] = '
                    f'{{ "{self._esc(dl.character)}", '
                    f'"{self._esc(dl.text)}", '
                    f'{dl.bg_index}{bg_cmt}, '
                    f'{prev_ptr}, '
                    f'{next_ptr}, '
                    f'{sel_block} }};{label_cmt}'
                )

            out += ["}", ""]

        return "\n".join(out)


# CLI

def main():
    ap = argparse.ArgumentParser(
        description="Compile a .dlg dialogue script to C++ (DialogueController format)"
    )
    ap.add_argument("input", help="Input .dlg / .md / .txt file")
    ap.add_argument("-o", "--output", default=None,
        help="Output base name (default: input filename without extension)")
    ap.add_argument("--stdout", action="store_true",
        help="Print generated code to stdout instead of writing files")
    args = ap.parse_args()

    try:
        with open(args.input, "r", encoding="utf-8") as f:
            src = f.read()
    except FileNotFoundError:
        print(f"ERROR: File not found: {args.input}", file=sys.stderr)
        sys.exit(1)

    base = args.output or os.path.splitext(os.path.basename(args.input))[0]
    scene_name = re.sub(r"[^A-Za-z0-9_]", "_", base).lower()

    try:
        p = DialogueParser(src)
        interactions = p.parse()
    except ParseError as e:
        print(f"PARSE ERROR: {e}", file=sys.stderr)
        sys.exit(1)

    if not interactions:
        print("WARNING: No [interaction] blocks found.", file=sys.stderr)
        sys.exit(0)

    # bg count warnings
    warnings = []
    for ia in interactions:
        nb = len(ia.bg_order)
        if nb > MAX_BG_SLOTS:
            warnings.append(
                f"  WARNING: '{ia.name}' uses {nb} distinct bgs "
                f"(DS hardware max is {MAX_BG_SLOTS}).\n"
                f"           Bgs: {', '.join(ia.bg_order)}\n"
                f"           Split into multiple interactions or reuse bg names."
            )

    gen = CodeGenerator(interactions, scene_name)
    cpp = gen.generate_cpp()
    h   = gen.generate_h()

    total_lines = sum(len(ia.lines) for ia in interactions)

    if args.stdout:
        print("// ===== .h =====")
        print(h)
        print("// ===== .cpp =====")
        print(cpp)
    else:
        h_path   = f"{base}_dialogue.h"
        cpp_path = f"{base}_dialogue.cpp"
        with open(h_path,   "w", encoding="utf-8") as f: f.write(h)
        with open(cpp_path, "w", encoding="utf-8") as f: f.write(cpp)

        print(f"Written:       {h_path}  /  {cpp_path}")
        print(f"Interactions:  {len(interactions)}")
        print(f"Total lines:   {total_lines}")
        print()

        if warnings:
            print("── WARNINGS " + "─" * 50)
            for w in warnings:
                print(w)
            print()

        print(f"Setup:")
        print(f"  {scene_name}_dialogue_bg_slot = bgInit(3, BgType_Text8bpp, BgSize_T_256x256, 0, 1);")
        print()
        print(f"Per-interaction:")
        for ia in interactions:
            vp = f"{scene_name}_{ia.name}"
            nb = len(ia.bg_order)
            print(f"  {vp}_load();")
            print(f"  dialogueCtrl.setLoader({vp}_load_bg);")
            print(f"  dialogueCtrl.start({vp}_first());")
            print(f"  // ... on finish: {vp}_unload();")
            if nb > MAX_BG_SLOTS:
                print(f"  // !! {nb} bgs in this interaction — exceeds DS limit")
            print()


if __name__ == "__main__":
    main()