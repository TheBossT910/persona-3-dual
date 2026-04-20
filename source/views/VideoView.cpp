#include <nds.h>
#include <stdio.h>
#include "core/globals.h"
#include "VideoView.h"

# define FRAME_SIZE (256 * 192 * 2)

FILE* videoFile;
u16* ramBuffer;
int pulldownState;
int currentFrame;
int bg;

void VideoView::Init() {
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
    musicCtrl.init("nitro:/video/intro.mp3", 0.0f, -1.0f);

    // open file and allocate memory
    videoFile = fopen("nitro:/video/intro.raw", "rb");
    if (videoFile == NULL) {
        consoleDemoInit();
        iprintf("ERROR: video_24fps.raw not found!\nCheck nitroFSInit() and path.");
        while(1) swiWaitForVBlank();
    }

    ramBuffer = (u16*)malloc(FRAME_SIZE);
    pulldownState = 0;

    // initial read
    fread(ramBuffer, 1, FRAME_SIZE, videoFile);
}

ViewState VideoView::Update() {
    musicCtrl.update();

    scanKeys();
    int keys = keysDown();

    // transition to intro on any input
    if (keys) {
        musicCtrl.pause();
        // transition both screens to black
        for(int i = 0; i <= 16; i++) {
            setBrightness(3, -i);

            // wait a few frames
            for (int duration = 0; duration <= 2; duration++) {
                swiWaitForVBlank();
            }
        }
        return ViewState::INTRO;
    }

    // check where the video should be right now
    // 24 frames per second * current audio time
    int expectedFrame = (int)(musicCtrl.getTime() * 24.0f);

    // are we ahead of the audio? Just wait and don't draw.
    if (currentFrame > expectedFrame) {
        swiWaitForVBlank(); 
        return ViewState::KEEP_CURRENT;
    }

    // are we severely behind the audio? (drop a frame to catch up)
    // if the SD card lagged and we are more than 1 frame behind, skip rendering.
    if (currentFrame < expectedFrame - 1) {
        // Skip reading 98KB, just jump the file pointer forward
        fseek(videoFile, FRAME_SIZE, SEEK_CUR);
        currentFrame++;
        return ViewState::KEEP_CURRENT; 
    }

    // we are perfectly in sync. draw the frame
    swiWaitForVBlank(); // wait for screen refresh to prevent tearing
    dmaCopy(ramBuffer, bgGetGfxPtr(bg), FRAME_SIZE);
    
    // read the next frame immediately
    size_t bytesRead = fread(ramBuffer, 1, FRAME_SIZE, videoFile);
    if (bytesRead < FRAME_SIZE) return ViewState::INTRO; // EOF
    
    currentFrame++;

    return ViewState::KEEP_CURRENT;
}

void VideoView::Cleanup() {
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