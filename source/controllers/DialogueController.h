#include <stdint.h>
#include <string>
#include <vector>
using namespace std;

// declare dialogue struct exists
struct dialogue;
struct dialogueSelection {
    string optionText;
    dialogue* next;     // where to lead on select
};
struct dialogue {
    string characterName;
    string text;
    int imageId;    // image to display
    dialogue* prev; // point to previous dialogue
    dialogue* next; // point to next dialogue
    bool isSelection;                     // if the current dialogue has selection options
    vector<dialogueSelection> selections; // selection options
};

class DialogueController {
    public:
        DialogueController();
        void Update();
        void DialogueDemo(int demoImageId);
};