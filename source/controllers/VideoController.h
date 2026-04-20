#pragma once
#include <nds.h>
#include "core/View.h"
#include "core/globals.h"
#include <string>

# define FRAME_SIZE (256 * 192 * 2)

using namespace std;

class VideoController {    
    public:
        VideoController() {};
        void init(string iFileName, ViewState iNextState, bool iIsSkippable);
        ViewState update();
        void cleanup();
    
    private:
        ViewState nextState;
        bool isSkippable;

        FILE* videoFile;
        u16* ramBuffer;
        int pulldownState;
        int currentFrame;
        int bg;  
};