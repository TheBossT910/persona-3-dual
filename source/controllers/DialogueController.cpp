#include <nds.h>
#include <stdio.h>
#include "DialogueController.h"

DialogueController::DialogueController() {}

void DialogueController::Update() {}

void DialogueController::DialogueDemo(int demoImageId) {
    // setup dialogue (3 lines)
    dialogue lines[3] = {};
    lines[0] = { "Akihiko", "text line 1", demoImageId, NULL, &lines[1], false, {} };
    lines[1] = { "Akihiko", "text line 2", demoImageId, &lines[0], &lines[2], false, {} };
    lines[2] = { "Akihiko", "text line 3", demoImageId, &lines[1], NULL, false, {} };
    
    // pointer to the currently displayed dialogue
    dialogue* currentDialogue = &lines[0];   

    consoleClear();
    while (true) {
        // display data
        bgShow(currentDialogue->imageId);
        iprintf("\x1b[12;16H%s", currentDialogue->text.c_str());

        scanKeys();
        u32 keys = keysHeld();

        if (keys & KEY_A) {
            consoleClear();
            bgHide(currentDialogue->imageId);
            // go to the next dialogue
            currentDialogue = currentDialogue->next;
        } else if (keys & KEY_B) {
            consoleClear();
            bgHide(currentDialogue->imageId);
            // go to the previous dialogue
            currentDialogue = currentDialogue->prev;
        } 
        
        if ((currentDialogue == NULL) || (keys & KEY_START)) {
            return;
        }

        // delay between input and display
        for (int i = 0; i < 6; i++) {
            swiWaitForVBlank();
        }
    }
}