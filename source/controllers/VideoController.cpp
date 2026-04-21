#include <nds.h>
#include <stdio.h>
#include <malloc.h>
#include "core/globals.h"
#include "VideoController.h"

void VideoController::init(string iFileName, float iFps,
                           ViewState iNextState, bool iIsSkippable) {
    nextState    = iNextState;
    fps          = iFps;
    isSkippable  = iIsSkippable;
    fileEOF      = false;

    // Use single interweaved file
    string videoPath = "nitro:/video/" + iFileName + ".vid";

    readIndex       = 0;
    writeIndex      = 0;
    framesAvailable = 0;
    currentFrame    = 0;

    videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
    videoSetModeSub(MODE_0_2D);

    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
    vramSetBankD(VRAM_D_MAIN_BG_0x06020000);
    vramSetBankC(VRAM_C_SUB_BG);

#if BYTES_PER_PX == 1
    bg = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
#else
    bgExtPaletteEnable();
    bg = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
#endif

    // initialize the ring buffer for video audio
    musicCtrl.initVideoAudio();

    videoFile = fopen(videoPath.c_str(), "rb");
    if (!videoFile) {
        consoleDemoInit();
        iprintf("ERR: %s", videoPath.c_str());
        while (1) swiWaitForVBlank();
    }

#if BYTES_PER_PX == 1
    u16 palette[256];
    size_t palRead = fread(palette, 2, 256, videoFile);
    if (palRead != 256) {
        consoleDemoInit();
        iprintf("ERR: palette read failed");
        while (1) swiWaitForVBlank();
    }
    
    for (int i = 0; i < 256; i++) {
        BG_PALETTE[i] = palette[i];
    }
#endif

    ramBuffer = (u8*)memalign(32, BUFFER_SIZE);
    if (!ramBuffer) {
        consoleDemoInit();
        iprintf("ERR: malloc failed");
        while (1) swiWaitForVBlank();
    }

    refillBuffer();
    refillBuffer();
}

void VideoController::refillBuffer() {
    if (fileEOF || framesAvailable >= FRAMES_TO_BUFFER) return;

    u32 audioSize = 0;
    
    // read audio chunk size
    if (fread(&audioSize, 4, 1, videoFile) != 1) {
        fileEOF = true;
        return;
    }

    // read audio data and push to ring buffer immediately
    if (audioSize > 0) {
        fread(audioBuf, 1, audioSize, videoFile);
        musicCtrl.pushVideoAudio(audioBuf, audioSize);
    }

    // read video frame
    u8* dest = &ramBuffer[writeIndex * FRAME_SIZE];
    size_t bytes = fread(dest, 1, FRAME_SIZE, videoFile);

    if (bytes == FRAME_SIZE) {
        DC_FlushRange(dest, FRAME_SIZE);
        writeIndex = (writeIndex + 1) % FRAMES_TO_BUFFER;
        framesAvailable++;
    } else {
        fileEOF = true;
    }
}

ViewState VideoController::update() {
    musicCtrl.update();
    scanKeys();
    int keys = keysDown();

    if (isSkippable && keys) {
        musicCtrl.pause();
        for (int i = 0; i <= 16; i++) {
            setBrightness(3, -i);
            musicCtrl.update();
            swiWaitForVBlank();
        }
        return nextState;
    }

    for (int r = 0; r < READS_PER_UPDATE; r++) {
        refillBuffer();
    }

    int expectedFrame = (int)(musicCtrl.getVideoTime() * fps);

    if (currentFrame > expectedFrame && !fileEOF) {
        swiWaitForVBlank();
        return ViewState::KEEP_CURRENT;
    }

    int dropBudget = 3;
    while (currentFrame < expectedFrame - 1 && framesAvailable > 0 && dropBudget-- > 0) {
        readIndex = (readIndex + 1) % FRAMES_TO_BUFFER;
        framesAvailable--;
        currentFrame++;
    }

    if (fileEOF && framesAvailable == 0) {
        return nextState;
    }

    if (framesAvailable > 0) {
        swiWaitForVBlank();
        dmaCopy(
            &ramBuffer[readIndex * FRAME_SIZE],
            bgGetGfxPtr(bg),
            FRAME_SIZE
        );
        readIndex = (readIndex + 1) % FRAMES_TO_BUFFER;
        framesAvailable--;
        currentFrame++;
    }

    return ViewState::KEEP_CURRENT;
}

void VideoController::cleanup() {
    setBrightness(3, 0);
    consoleClear();

    vramSetBankA(VRAM_A_LCD);
    vramSetBankC(VRAM_C_LCD);
    vramSetBankD(VRAM_D_LCD);

    fclose(videoFile);
    free(ramBuffer);
    
    musicCtrl.cleanup();
}