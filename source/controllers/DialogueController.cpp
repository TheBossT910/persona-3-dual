#include <nds.h>
#include <stdio.h>
#include "DialogueController.h"

DialogueController::DialogueController() {}

void DialogueController::Update() {
    int line = 0;    
    while (true) {
        consoleClear();
        switch(line) {
            case 0:
                iprintf("\x1b[12;16HWhat's up?");
                break;
            case 1:
                iprintf("\x1b[12;16HTime to go?");
                break;
            case 2:
                iprintf("\x1b[12;16HDon't slack off.");
                break;
            default:
                return;
        }

        scanKeys();
        u32 keys = keysHeld();
        if (keys & KEY_A) {
            line++;
        } else if (keys & KEY_B) {
            line--;
        } if (keys & KEY_START) {
            return;
        }

        // delay between input and display
        for (int i = 0; i < 6; i++) {
            swiWaitForVBlank();
        }
    }
}