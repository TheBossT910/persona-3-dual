#include <nds.h>
#include <stdio.h>
#include "core/globals.h"
#include "VideoController.h"

void VideoController::init(string iFileName, float iFps, ViewState iNextState, bool iIsSkippable) {
    // set variables
    nextState = iNextState;
    fps = iFps;
    isSkippable = iIsSkippable;
    string musicPath = "nitro:/video/" + iFileName + ".pcm";
    string videoPath = "nitro:/video/" + iFileName + ".raw";

    // initialize ring buffer
    readIndex = 0;
    writeIndex = 0;
    framesAvailable = 0;
    currentFrame = 0;

    // set video mode for 2 text layers and 2 extended rotation layer
	videoSetMode(MODE_5_2D);
	// set sub video mode for 4 text layers
	videoSetModeSub(MODE_0_2D);

	// map vram bank A and D to main engine background (slot 0)
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	vramSetBankD(VRAM_D_MAIN_BG_0x06020000);
    // map vram bank B to sub engine background
    vramSetBankC(VRAM_C_SUB_BG);

	// enable extended palettes
	bgExtPaletteEnable();
    bgExtPaletteEnableSub();

    // initialize background
    bg = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0); 

    // point to music
    musicCtrl.init(musicPath.c_str(), 0.0f, -1.0f);

    // open video file file
    videoFile = fopen(videoPath.c_str(), "rb");
    if (videoFile == NULL) {
        consoleDemoInit();
        iprintf(videoPath.c_str());
        while(1) swiWaitForVBlank();
    }

    // allocate memory
    ramBuffer = (u16*)malloc(BUFFER_SIZE);

    // prefill the entire buffer before the video starts playing
    for (int i = 0; i < FRAMES_TO_BUFFER; i++) {
        size_t bytes = fread(&ramBuffer[writeIndex * (FRAME_SIZE / 2)], 1, FRAME_SIZE, videoFile);
        if (bytes == FRAME_SIZE) {
            writeIndex = (writeIndex + 1) % FRAMES_TO_BUFFER;
            framesAvailable++;
        }
    }
}

ViewState VideoController::update() {
    musicCtrl.update();
    scanKeys();
    int keys = keysDown();

    // transition both screens to black on any input
    if (isSkippable && keys) {
        musicCtrl.pause();
        for(int i = 0; i <= 16; i++) {
            setBrightness(3, -i);

            for (int duration = 0; duration <= 2; duration++) {
                musicCtrl.update();
                swiWaitForVBlank();
            }
        }
        return nextState;
    }

    // refill the buffer if we have space
    if (framesAvailable < FRAMES_TO_BUFFER) {
        size_t bytes = fread(&ramBuffer[writeIndex * (FRAME_SIZE / 2)], 1, FRAME_SIZE, videoFile);
        
        if (bytes == FRAME_SIZE) {
            writeIndex = (writeIndex + 1) % FRAMES_TO_BUFFER;
            framesAvailable++;
        } else {
            // if we didn't read a full frame, we hit the end of the file.
            // we shouldn't quit immediately though, because we might still have 
            // frames sitting in framesAvailable waiting to be drawn
            if (framesAvailable == 0) {
                return nextState; 
            }
        }
    }

    // check where the video should be right now
    int expectedFrame = (int)(musicCtrl.getTime() * fps);

    // are we ahead of the audio? just wait and don't draw.
    if (currentFrame > expectedFrame) {
        swiWaitForVBlank(); 
        return ViewState::KEEP_CURRENT;
    }

    // severely behind audio? drop a frame
    if (currentFrame < expectedFrame - 1) {
        // only skip if we actually have frames in the buffer to skip
        if (framesAvailable > 0) {
            readIndex = (readIndex + 1) % FRAMES_TO_BUFFER;
            framesAvailable--;
            currentFrame++;
        }
        return ViewState::KEEP_CURRENT; 
    }

    // draw the frame
    if (framesAvailable > 0) {
        swiWaitForVBlank(); 
        
        dmaCopy(&ramBuffer[readIndex * (FRAME_SIZE / 2)], bgGetGfxPtr(bg), FRAME_SIZE);
        
        readIndex = (readIndex + 1) % FRAMES_TO_BUFFER;
        framesAvailable--;
        currentFrame++;
    }

    return ViewState::KEEP_CURRENT;
}

void VideoController::cleanup() {
    // clear screen
    setBrightness(3, 0);
    consoleClear();

    // reset vram
    vramSetBankA(VRAM_A_LCD);
    vramSetBankC(VRAM_C_LCD);
    vramSetBankD(VRAM_D_LCD);

    fclose(videoFile);
    free(ramBuffer);
}