#include <nds.h>
#include <stdio.h>
#include "DialogueController.h"

DialogueController::DialogueController() {}

void DialogueController::start(dialogue* firstLine) {
    current        = firstLine;
    optionCount    = 0;
    selectedOption = 0;
    active         = true;
    isDisplayed    = false;
    doRenderOptions= false;
    animIndex      = 0;
    animWait       = 0;
    animDone       = false;
    prevKeys       = 0;
    consoleClear();
}

// Returns true while dialogue is still running, false when it finishes
bool DialogueController::update(u32 keys) {
    if (!active || current == nullptr) {
        active = false;
        return false;
    }

    // Fresh key presses only, ignore held keys
    u32 pressed = keys & ~prevKeys;
    prevKeys = keys;

    // --- Step 1: Show the dialogue line (animate text) ---
    if (!isDisplayed) {
        bgShow(current->imageId);

        // Animate one character per frame
        if (!animDone) {
            if (animIndex <= (int)current->text.length()) {
                iprintf("\x1b[12;16H%s \n",
                    current->text.substr(0, animIndex).c_str());
                animIndex++;
            } else {
                animDone = true;
                isDisplayed   = true;
                optionCount   = current->selections.size();
                doRenderOptions = (optionCount > 0);
            }
            return true; // Still animating, don't process input yet
        }
    }

    // --- Step 2: Render options if needed ---
    if (doRenderOptions) {
        renderOptions();
        doRenderOptions = false;
    }

    // --- Step 3: Handle exit ---
    if (keys & KEY_START) {
        bgHide(current->imageId);
        consoleClear();
        active = false;
        return false;
    }

    // --- Step 4: Handle input ---
    if (optionCount > 0) {
        // Selection dialogue
        if (pressed & KEY_DOWN) {
            selectedOption = (selectedOption + 1) % optionCount;
            doRenderOptions = true;
        } else if (pressed & KEY_UP) {
            selectedOption = (selectedOption + optionCount - 1) % optionCount;
            doRenderOptions = true;
        } else if (pressed & KEY_A) {
            bgHide(current->imageId);
            current        = current->selections[selectedOption].next;
            selectedOption = 0;
            isDisplayed    = false;
            animIndex      = 0;
            animDone       = false;
            consoleClear();
        }
    } else {
        // Linear dialogue
        if (pressed & KEY_A) {
            bgHide(current->imageId);
            current     = current->next;
            isDisplayed = false;
            animIndex   = 0;
            animDone    = false;
            consoleClear();
        } else if (pressed & KEY_B) {
            bgHide(current->imageId);
            current     = current->prev;
            isDisplayed = false;
            animIndex   = 0;
            animDone    = false;
            consoleClear();
        }
    }

    // Check if we've run out of dialogue
    if (current == nullptr) {
        active = false;
        consoleClear();
        return false;
    }

    return true;
}

void DialogueController::renderOptions() {
    consoleClear();
    iprintf("\x1b[12;16H%s\n", current->text.c_str());
    for (int i = 0; i < optionCount; i++) {
        iprintf("%c %s\n",
            i == selectedOption ? '>' : ' ',
            current->selections[i].text.c_str());
    }
}