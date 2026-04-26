#pragma once
#include <stdint.h>
#include <string>
#include <vector>
using namespace std;

struct dialogue;
struct dialogueSelection {
    string text;
    bool isSelected;
    dialogue* next;
};
struct dialogue {
    string characterName;
    string text;
    int imageId;        // index into the active interaction's bg_loaders[] table
    dialogue* prev;
    dialogue* next;
    vector<dialogueSelection> selections;
};

class DialogueController {
    public:
        DialogueController();
        void start(dialogue* firstLine);
        void update(u32 keys);
        bool isActive() const { return active; }

        // Set the bg-swap callback for the current interaction.
        // Call this before start(). The function receives the imageId index
        // and is responsible for loading the correct tiles into the shared slot.
        // Generated signature per interaction: void scene_ia_load_bg(int bgIndex);
        void setLoader(void (*loader)(int bgIndex)) { bgLoader = loader; }

    private:
        void renderText();
        void renderOptions();

        dialogue* current         = nullptr;
        int       optionCount     = 0;
        int       selectedOption  = 0;
        bool      active          = false;
        bool      isDisplayed     = false;
        bool      doRenderOptions = false;

        // bg-swap callback — called whenever a new line becomes current
        void (*bgLoader)(int bgIndex) = nullptr;

        // for text animation
        int    animIndex    = 0;
        int    animWait     = 0;
        bool   animDone     = false;

        // for input debounce
        u32    prevKeys     = 0;
};