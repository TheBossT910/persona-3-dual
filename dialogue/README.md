# Dialogue Compiler — Complete Reference

## Overview

`dialogue_compiler.py` takes a `.dlg` script and produces a `.h` + `.cpp` pair
for your `DialogueController` system. One file = one scene. Each scene contains
any number of named **interaction** blocks.

---

## Quick Start

```bash
python dialogue_compiler.py park.dlg          # -> park_dialogue.h / .cpp
python dialogue_compiler.py park.dlg -o mypark
python dialogue_compiler.py park.dlg --stdout
```

**In your game code:**

```cpp
#include "park_dialogue.h"

// In scene init — call once before any dialogue runs
park_init_all();

// Start a specific interaction (e.g. when player walks up to an NPC)
dialogueCtrl.start(park_akihiko_meets_yuki_first());
dialogueCtrl.start(park_merchant_greeting_first());
dialogueCtrl.start(park_notice_board_first());
```

---

## Core Concept: Interactions

A **scene** file holds as many independent dialogue graphs as you need —
one per NPC, one per sign, one per cutscene, etc. Each is wrapped in a
named `[interaction]` block.

Each interaction compiles to:
- Its own `dialogue` array (no index bleed between interactions)
- Its own `_init()` and `_first()` accessor
- Inline accessors for every `@label` defined inside it

Labels are **scoped to their interaction** — the same label name (e.g. `@decline`)
can appear in multiple interactions without collision.

---

## File Structure

```
# Top-level: comments, section headers, scene-wide @bg defaults. Should match the bg file name.
@bg Akihiko  bgAkihiko
@bg Yuki     bgYuki

[interaction: npc_one | Optional human description]
    # ... dialogue for this NPC ...

[interaction: npc_two | Another NPC]
    # ... dialogue for this NPC ...
```

---

## Full Syntax Reference

### Comments
```
# Anything after a # is ignored
```

### Section headers (cosmetic)
```
== Scene: The Park ==
```
Purely for readability in large files. Ignored by the compiler.

---

### `@bg` — Set default background for a character

```
@bg CharacterName  bgVariableName
```

- **At the top of the file (before any `[interaction]`):** scene-wide default.
  All interactions inherit it.
- **Inside an `[interaction]` block:** overrides for that interaction only.
- If a character has no `@bg` at all, the compiler auto-derives a name:
  `"Akihiko"` becomes `bgAkihiko`.

---

### `[interaction: name | description]`

```
[interaction: akihiko_talk | Akihiko by the fountain]
```

Starts a new interaction block. Everything until the next `[interaction]`
or end-of-file belongs to it.

- **`name`**: becomes the C++ identifier. Use `snake_case`.
  Produces `scene_name_first()`, `scene_name_init()`, etc.
- **`| description`**: optional human-readable comment written into the
  generated `.cpp` as a section header.

---

### Dialogue lines

```
CHARACTER: Text the character says.
```

Uses the current default bg for `CHARACTER`.

**One-off bg override** for a single line without changing the default:
```
Yuki(bgYukiClose): I come here to think.
```

`CHARACTER` must start with a letter. Spaces are allowed (`Old Man: ...`).

---

### `@label` — Name a line

```
@label_name
CHARACTER: This line is now reachable by name.
```

- Must be placed immediately before the target line.
- Becomes an inline accessor in the header:
  `park_hana_request_hana_agree()` returns a pointer to that line.
- Scoped to its interaction; the same name in another interaction is fine.

---

### `[choice]` — Selection node

```
CHARACTER: Question text?

[choice]
    > Option A  -> @label_a
    > Option B  -> @label_b
    > Option C  -> @label_c
```

- Place `[choice]` immediately after the question line.
- The question line becomes the selection node (`next = NULL`, `selections` populated).
- Each `-> @label` must point to a label defined in the **same** interaction.
- Indentation of `>` lines is optional but recommended for readability.

---

### `[end]` — Explicit dead-end

```
CHARACTER: Goodbye.
[end]
```

Marks the previous line's `next = NULL`. Required for branch termini inside
an interaction. The very last line of an interaction is automatically a
dead-end, so `[end]` is only needed inside branches.

---

### `[jump @label]` — Unconditional redirect

```
CHARACTER: See you tomorrow.
[jump @day_start]
```

Overrides the `next` pointer of the previous line to point at `@label`
instead of the physically following line. Useful for merging branches or
looping back.

---

## All Syntax at a Glance

| Syntax | Meaning |
|---|---|
| `# text` | Comment |
| `== Title ==` | Section header (cosmetic) |
| `@bg Char bgVar` | Default background for character |
| `[interaction: name \| desc]` | Start an interaction block |
| `CHAR: text` | Dialogue line (default bg) |
| `CHAR(bgVar): text` | Dialogue line with one-off bg override |
| `@label` | Name the next line |
| `[choice]` | Make previous line a selection node |
| `> Text -> @label` | Choice option (inside `[choice]`) |
| `[end]` | Previous line's next = NULL |
| `[jump @label]` | Previous line's next points to @label |

---

## Generated Output

### `.h` file

For each interaction:
```cpp
extern dialogue park_akihiko_meets_yuki_lines[8];
void  park_akihiko_meets_yuki_init();
inline dialogue* park_akihiko_meets_yuki_first() { ... }
inline dialogue* park_akihiko_meets_yuki_some_label() { ... }  // per @label
```

Plus a scene-wide convenience initialiser:
```cpp
inline void park_init_all() {
    park_akihiko_meets_yuki_init();
    park_hana_request_init();
    // ...
}
```

### `.cpp` file

One array + one `_init()` function per interaction. All `prev`/`next`
pointers and `selections` vectors are wired up. Selections are declared as
local variables inside `_init()`, matching your hand-written style exactly.

---

## Usage Patterns

### Trigger dialogue when the player interacts with an NPC or object

```cpp
if (playerTouches(npcAkihiko)) {
    dialogueCtrl.start(park_akihiko_meets_yuki_first());
}
if (playerTouches(noticeBoard)) {
    dialogueCtrl.start(park_notice_board_first());
}
```

### Conditional entry point (skip to a different part of the interaction)

```cpp
// If the player already accepted Hana's quest, start from her thank-you line
if (questAccepted) {
    dialogueCtrl.start(park_hana_request_hana_agree());
} else {
    dialogueCtrl.start(park_hana_request_first());
}
```

### Check if dialogue is still running in your update loop

```cpp
void sceneUpdate(u32 keys) {
    if (dialogueCtrl.isActive()) {
        dialogueCtrl.update(keys);
    }
}
```

---

## Tips

**One `.dlg` per scene/view.** `park.dlg`, `market.dlg`, `inn.dlg`. Each gets
its own namespace so there are no naming collisions between scenes.

**Only label what you need to jump to.** Auto-generated line names (`@line_0`)
don't appear in the header. Only your custom `@labels` become accessors.

**`@bg` overrides are interaction-local.** Writing `@bg Yuki bgYukiNight`
inside one interaction does not affect Yuki's background in other interactions.

**Multiple characters per interaction is natural.** Just write each character's
lines in sequence. Backgrounds switch per-character or per-line as needed.

**Signs, narration, environmental text:** Use any "character" name —
`Sign`, `Narrator`, `[System]`. Pair with a per-line bg override.

**Never edit the generated files.** They are fully regenerated on every
compile. Your source of truth is the `.dlg`.
