#pragma once
#include <nds.h>
#include "core/View.h"
#include "core/globals.h"
#include <string>

#define FRAME_SIZE (256 * 192 * 2)
#define FRAMES_TO_BUFFER 10
#define BUFFER_SIZE (FRAME_SIZE * FRAMES_TO_BUFFER)

using namespace std;

class VideoController {    
    public:
        VideoController() {};
        void init(string iFileName, float iFps, ViewState iNextState, bool iIsSkippable);
        ViewState update();
        void cleanup();
    
    private:
        ViewState nextState;
        bool isSkippable;
        float fps;

        FILE* videoFile;
        int currentFrame;
        int bg;

        // buffer
        u16* ramBuffer;
        int readIndex;
        int writeIndex;
        int framesAvailable;
};