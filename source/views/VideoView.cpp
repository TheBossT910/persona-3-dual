#include <nds.h>
#include <stdio.h>
#include "core/globals.h"
#include "VideoView.h"

# define FRAME_SIZE (256 * 192 * 2)

FILE* videoFile;
u16* ramBuffer;
int pulldownState;
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

    // open file and allocate memory
    videoFile = fopen("nitro:/video/video_24fps.raw", "rb");
    if (videoFile == NULL) {
        consoleDemoInit();
        iprintf("ERROR: video_24fps.raw not found!\nCheck nitroFSInit() and path.");
        while(1) swiWaitForVBlank();
    }

    ramBuffer = (u16*)malloc(FRAME_SIZE);
    pulldownState = 0;

    // initial read
    fread(ramBuffer, 1, FRAME_SIZE, videoFile);

    // setting the alpha bit
    for (int i = 0; i < (FRAME_SIZE / 2); i++) {
        ramBuffer[i] |= 0x8000; 
    }
}

ViewState VideoView::Update() {
    // determine how many VBlanks to wait for this specific frame
    int targetVblanks = (pulldownState == 0) ? 3 : 2;
    for (int i = 0; i < targetVblanks; i++) {
        swiWaitForVBlank();
    }

    // copy data into background
    dmaCopy(ramBuffer, bgGetGfxPtr(bg), FRAME_SIZE);

    // read next frame
    size_t bytesRead = fread(ramBuffer, 1, FRAME_SIZE, videoFile);
    if (bytesRead < FRAME_SIZE) return ViewState::INTRO;    // end of file

    // set the alpha bit
    for (int i = 0; i < (FRAME_SIZE / 2); i++) {
        ramBuffer[i] |= 0x8000; 
    }

    // flip the state between 1/0
    pulldownState = !pulldownState;

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