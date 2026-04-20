#include <nds.h>
#include <stdio.h>
#include "core/globals.h"
#include "VideoController.h"

void VideoController::init(string iFileName, float iFps, ViewState iNextState, bool iIsSkippable) {
    // set variables
    nextState = iNextState;
    fps = iFps;
    isSkippable = iIsSkippable;
    string musicPath = "nitro:/video/" + iFileName + ".mp3";
    string videoPath = "nitro:/video/" + iFileName + ".raw";

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

    ramBuffer = (u16*)malloc(FRAME_SIZE);
    pulldownState = 0;

    // initial read
    fread(ramBuffer, 1, FRAME_SIZE, videoFile);
}

ViewState VideoController::update() {
    musicCtrl.update();

    scanKeys();
    int keys = keysDown();

    // transition to intro on any input
    if (isSkippable && keys) {
        musicCtrl.pause();
        // transition both screens to black
        for(int i = 0; i <= 16; i++) {
            setBrightness(3, -i);

            // wait a few frames
            for (int duration = 0; duration <= 2; duration++) {
                swiWaitForVBlank();
            }
        }
        return nextState;
    }

    // check where the video should be right now
    // frames per second * current audio time
    int expectedFrame = (int)(musicCtrl.getTime() * fps);

    // are we ahead of the audio? Just wait and don't draw.
    if (currentFrame > expectedFrame) {
        swiWaitForVBlank(); 
        return ViewState::KEEP_CURRENT;
    }

    // are we severely behind the audio? (drop a frame to catch up)
    // if the SD card lagged and we are more than 1 frame behind, skip rendering.
    if (currentFrame < expectedFrame - 1) {
        // skip reading 98KB, just jump the file pointer forward
        fseek(videoFile, FRAME_SIZE, SEEK_CUR);
        currentFrame++;
        return ViewState::KEEP_CURRENT; 
    }

    // we are perfectly in sync. draw the frame
    swiWaitForVBlank(); // wait for screen refresh to prevent tearing
    dmaCopy(ramBuffer, bgGetGfxPtr(bg), FRAME_SIZE);
    
    // read the next frame immediately
    size_t bytesRead = fread(ramBuffer, 1, FRAME_SIZE, videoFile);
    if (bytesRead < FRAME_SIZE) return nextState; // EOF
    
    currentFrame++;

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