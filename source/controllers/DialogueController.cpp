#include <nds.h>
#include <stdio.h>
#include "DialogueController.h"

DialogueController::DialogueController() {}

void DialogueController::Update() {}

void DialogueController::DialogueDemo(int demoImageId) {
    // setup dialogue (3 lines)
    dialogue lines[3] = {};
    lines[0] = { "Akihiko", "text line 1", demoImageId, NULL, &lines[1], {} };
    lines[1] = { "Akihiko", "text line 2", demoImageId, &lines[0], &lines[2], {} };

    // selection example
    dialogue selection_1_dia = { "Akihiko", "selection 1", demoImageId, &lines[2], NULL, {} };
    dialogue selection_2_dia = { "Akihiko", "selection 2", demoImageId, &lines[2], NULL, {} };
    dialogueSelection selection_1 = { "Go home", false, &selection_1_dia };
    dialogueSelection selection_2 = { "Go for a walk", false, &selection_2_dia };

    lines[2] = { "Akihiko", "line 3, sel", demoImageId, &lines[1], NULL, { selection_1, selection_2 } };

    // pointer to the currently displayed dialogue
    dialogue* currentDialogue = &lines[0];   
    int optionCount = 0;
    int selectedOption = 0;

    consoleClear();
    while (true) {
        optionCount = currentDialogue->selections.size();

        // display data
        bgShow(currentDialogue->imageId);
        iprintf("\x1b[12;16H%s\n", currentDialogue->text.c_str());
        for (int option = 0; option < optionCount; option++) {
            // display options
            iprintf("%c %s\n", option == selectedOption ? '>' : ' ', currentDialogue->selections[option].text.c_str());
        }

        scanKeys();
        u32 keys = keysHeld();

        // clear data
        if ((keys & KEY_A) || (keys & KEY_B)) {
            consoleClear();
            bgHide(currentDialogue->imageId);
        }
        
        // user control
        if (optionCount && (keys & KEY_DOWN)) {
            // select next option
            selectedOption = (selectedOption + 1) % optionCount;
        } else if (optionCount && (keys & KEY_UP)) {
            // select previous option
            selectedOption = (selectedOption + optionCount - 1) % optionCount;
        } else if (optionCount && (keys & KEY_A)) {
            // go to next selection dialogue
            currentDialogue = currentDialogue->selections[selectedOption].next;
        } else if (!optionCount && (keys & KEY_A)) {
            // go to next dialogue
            currentDialogue = currentDialogue->next;
        } else if (keys & KEY_B) {
            // go to previous dialogue
            currentDialogue = currentDialogue->prev;
        } 

        // exit function
        if ((currentDialogue == NULL) || (keys & KEY_START)) {
            return;
        }

        // delay between input and display
        for (int i = 0; i < 6; i++) {
            swiWaitForVBlank();
        }
    }
}