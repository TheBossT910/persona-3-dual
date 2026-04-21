// VideoController.cpp
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

    string musicPath = "nitro:/video/" + iFileName + ".pcm";
    string videoPath = "nitro:/video/" + iFileName + ".raw";

    readIndex      = 0;
    writeIndex     = 0;
    framesAvailable = 0;
    currentFrame   = 0;

    // ── Video mode ───────────────────────────────────────────────────────────
    videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
    videoSetModeSub(MODE_0_2D);

    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
    vramSetBankD(VRAM_D_MAIN_BG_0x06020000);
    vramSetBankC(VRAM_C_SUB_BG);

    // ── Background ───────────────────────────────────────────────────────────
#if BYTES_PER_PX == 1
    // 8bpp bitmap background — uses standard BG_PALETTE, not extended palettes
    bg = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
#else
    bgExtPaletteEnable();
    bg = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
#endif

    // ── Audio ────────────────────────────────────────────────────────────────
    musicCtrl.init(musicPath.c_str(), 0.0f, 0.0f);

    // ── File ─────────────────────────────────────────────────────────────────
    videoFile = fopen(videoPath.c_str(), "rb");
    if (!videoFile) {
        consoleDemoInit();
        iprintf("ERR: %s", videoPath.c_str());
        while (1) swiWaitForVBlank();
    }

    // ── Palette (8bpp only) ──────────────────────────────────────────────────
    // The .raw file starts with a 512-byte palette header (256 × BGR555 u16).
    // Read it once and load it into the main engine BG_PALETTE before any
    // frames are drawn. BgType_Bmp8 reads from BG_PALETTE, not ext palettes.
#if BYTES_PER_PX == 1
    u16 palette[256];
    size_t palRead = fread(palette, 2, 256, videoFile);
    if (palRead != 256) {
        consoleDemoInit();
        iprintf("ERR: palette read failed (%d/256)", (int)palRead);
        while (1) swiWaitForVBlank();
    }
    
    for (int i = 0; i < 256; i++) {
        BG_PALETTE[i] = palette[i];
    }
#endif

    // ── RAM buffer ───────────────────────────────────────────────────────────
    // memalign guarantees 32-byte alignment required for DMA
    ramBuffer = (u8*)memalign(32, BUFFER_SIZE);
    if (!ramBuffer) {
        consoleDemoInit();
        iprintf("ERR: malloc failed");
        while (1) swiWaitForVBlank();
    }

    // Pre-fill both slots before playback starts
    refillBuffer();
    refillBuffer();
}

// ── Internal: read one frame into the next write slot ────────────────────────
void VideoController::refillBuffer() {
    if (fileEOF || framesAvailable >= FRAMES_TO_BUFFER) return;

    u8* dest = &ramBuffer[writeIndex * FRAME_SIZE];
    size_t bytes = fread(dest, 1, FRAME_SIZE, videoFile);

    if (bytes == FRAME_SIZE) {
        // Flush data cache so DMA sees fresh data (critical on ARM9 hardware)
        DC_FlushRange(dest, FRAME_SIZE);
        writeIndex = (writeIndex + 1) % FRAMES_TO_BUFFER;
        framesAvailable++;
    } else {
        fileEOF = true;
    }
}

// ── Main update — called once per game loop iteration ────────────────────────
ViewState VideoController::update() {
    musicCtrl.update();
    scanKeys();
    int keys = keysDown();

    // ── Skip input ───────────────────────────────────────────────────────────
    if (isSkippable && keys) {
        musicCtrl.pause();
        for (int i = 0; i <= 16; i++) {
            setBrightness(3, -i);
            musicCtrl.update();
            swiWaitForVBlank();
        }
        return nextState;
    }

    // ── Refill buffer BEFORE sync — use VBlank wait time productively ────────
    for (int r = 0; r < READS_PER_UPDATE; r++) {
        refillBuffer();
    }

    // ── Sync check ───────────────────────────────────────────────────────────
    int expectedFrame = (int)(musicCtrl.getTime() * fps);

    // Ahead of audio — wait one VBlank, don't draw
    if (currentFrame > expectedFrame && !fileEOF) {
        swiWaitForVBlank();
        return ViewState::KEEP_CURRENT;
    }

    // Behind audio — drop frames to catch up (max 3 at once)
    int dropBudget = 3;
    while (currentFrame < expectedFrame - 1 && framesAvailable > 0 && dropBudget-- > 0) {
        readIndex = (readIndex + 1) % FRAMES_TO_BUFFER;
        framesAvailable--;
        currentFrame++;
    }

    // ── End of file with buffer drained ──────────────────────────────────────
    if (fileEOF && framesAvailable == 0) {
        return nextState;
    }

    // ── Draw frame ───────────────────────────────────────────────────────────
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

// ── Cleanup ──────────────────────────────────────────────────────────────────
void VideoController::cleanup() {
    setBrightness(3, 0);
    consoleClear();

    vramSetBankA(VRAM_A_LCD);
    vramSetBankC(VRAM_C_LCD);
    vramSetBankD(VRAM_D_LCD);

    fclose(videoFile);
    free(ramBuffer);
}