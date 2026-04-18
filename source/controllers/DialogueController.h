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
    int imageId;
    dialogue* prev;
    dialogue* next;
    vector<dialogueSelection> selections;
};

class DialogueController {
    public:
        DialogueController();

        // Call to begin a dialogue sequence
        void start(dialogue* firstLine);

        // Call every frame in your View's Update(), returns true while dialogue is active
        bool update(u32 keys);

        bool isActive() const { return active; }

    private:
        void renderText();
        void renderOptions();

        dialogue* current         = nullptr;
        int       optionCount     = 0;
        int       selectedOption  = 0;
        bool      active          = false;
        bool      isDisplayed     = false;
        bool      doRenderOptions = false;

        // For text animation
        int    animIndex    = 0;
        int    animWait     = 0;
        bool   animDone     = false;

        // For input debounce, only act on fresh presses
        u32    prevKeys     = 0;
};