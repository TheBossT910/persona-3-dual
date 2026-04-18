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

void DialogueController::update(u32 keys) {
    if (!active || current == NULL) {
        active = false;
        return;
    }

    // fresh key presses only, ignore held keys
    u32 pressed = keys & ~prevKeys;
    prevKeys = keys;

    // animate in the the dialogue
    if (!isDisplayed) {
        bgShow(current->imageId);

        // animate one character per frame
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

            return; // still animating, don't process input yet
        }
    }

    // render options if needed
    if (doRenderOptions) {
        renderOptions();
        doRenderOptions = false;
    }

    // handle exit
    if (keys & KEY_START) {
        bgHide(current->imageId);
        consoleClear();
        active = false;
        return;
    }

    // handle input
    if (optionCount > 0) {
        // selection dialogue
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

            // check if we've run out of dialogue
            if (current == NULL) {
                active = false;
                return;
            }
        }
    } else {
        // linear dialogue (no selection)
        if (pressed & KEY_A) {
            bgHide(current->imageId);
            current     = current->next;
            isDisplayed = false;
            animIndex   = 0;
            animDone    = false;
            consoleClear();

            // check if we've run out of dialogue
            if (current == NULL) {
                active = false;
                return;
            }
        } else if (pressed & KEY_B) {
            bgHide(current->imageId);
            current     = current->prev;
            isDisplayed = false;
            animIndex   = 0;
            animDone    = false;
            consoleClear();
            
            // check if we've run out of dialogue
            if (current == NULL) {
                active = false;
                return;
            }
        }
    }
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