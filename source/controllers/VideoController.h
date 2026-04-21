// VideoController.h
#pragma once
#include <nds.h>
#include "core/View.h"
#include "core/globals.h"
#include <string>
#include <fat.h>

// --- Tuning knobs ---
// Option A: Full res 16bpp (hardest on HW, ~98KB/frame)
// #define FRAME_W      256
// #define FRAME_H      192
// #define BYTES_PER_PX 2

// Option B: Full res 8bpp palette (recommended, ~49KB/frame)
#define FRAME_W         256
#define FRAME_H         192
#define BYTES_PER_PX    1

#define FRAME_SIZE      (FRAME_W * FRAME_H * BYTES_PER_PX)

// Double buffer — display one while reading the other
#define FRAMES_TO_BUFFER 2
#define BUFFER_SIZE     (FRAME_SIZE * FRAMES_TO_BUFFER)

// How many frames to try to read per update() call
// On HW, one fread per frame is safer; bump to 2 if you have headroom
#define READS_PER_UPDATE 1

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

        // Align to 32 bytes for DMA efficiency
        u8* ramBuffer;          // u8 for 8bpp; swap to u16* for 16bpp
        int readIndex;
        int writeIndex;
        int framesAvailable;

        void refillBuffer();
};