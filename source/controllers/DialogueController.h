#include <stdint.h>
#include <string>
#include <vector>
using namespace std;

// declare dialogue struct exists
struct dialogue;
struct dialogueSelection {
    string text;
    bool isSelected;
    dialogue* next;     // where to lead on select
};
struct dialogue {
    string characterName;
    string text;
    int imageId;    // image to display
    dialogue* prev; // point to previous dialogue
    dialogue* next; // point to next dialogue
    vector<dialogueSelection> selections; // selection options
};

class DialogueController {
    public:
        DialogueController();
        void update();
        void dialogueDemo(int demoImageId);
    private:
        void animateText(string text);
        void delay();
};