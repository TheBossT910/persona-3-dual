#pragma once
#include <nds.h>
#include "core/View.h"
#include "core/globals.h"
#include <string>
#include <fat.h>

#define FRAME_W         256
#define FRAME_H         192
#define BYTES_PER_PX    1
#define FRAME_SIZE      (FRAME_W * FRAME_H * BYTES_PER_PX)
#define FRAMES_TO_BUFFER 15
#define BUFFER_SIZE     (FRAME_SIZE * FRAMES_TO_BUFFER)
#define READS_PER_UPDATE 3

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
        bool fileEOF;
        int currentFrame;
        int bg;

        u8* ramBuffer;
        int readIndex;
        int writeIndex;
        int framesAvailable;
        
        u8 audioBuf[16384]; // Temp buffer for reading interwoven audio chunk

        void refillBuffer();
};