#include <nds.h>
#include <stdio.h>
#include "core/globals.h"
#include "DisclaimerView.h"

// assets
#include "cautionBackground.h"
#include "cautionBackgroundSub.h"

void DisclaimerView::Init() {
    // set video mode for 3 text layers and 1 extended rotation layer
	videoSetMode(MODE_3_2D);
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

	// set brightness on bottom screen to completely dark (no visible image)
	setBrightness(2, -16);

	// initialize backgrounds
	// check https://mtheall.com/vram.html to ensure bg fit in vram
	bg[0] = bgInit(2, BgType_Text8bpp, BgSize_T_256x256, 10, 3);    // caution (main screen)
    bg[1] = bgInitSub(1, BgType_Text8bpp, BgSize_T_256x256, 1, 2);  // caution (sub screen)

	// need to set priority to properly display
	// 0 is highest, 3 is lowest
    bgSetPriority(bg[0], 0);
    bgSetPriority(bg[1], 0);

    // reset background vram
    // 256x256 backgrounds use 2048 bytes of map memory
    dmaFillHalfWords(0, bgGetMapPtr(bg[0]), 2048);
    dmaFillHalfWords(0, bgGetMapPtr(bg[1]), 2048);

	// copy graphics to vram
  	dmaCopy(cautionBackgroundTiles, bgGetGfxPtr(bg[0]), cautionBackgroundTilesLen);
    dmaCopy(cautionBackgroundSubTiles,  bgGetGfxPtr(bg[1]), cautionBackgroundSubTilesLen);

	// copy maps to vram
  	dmaCopy(cautionBackgroundMap, bgGetMapPtr(bg[0]), cautionBackgroundMapLen);
    dmaCopy(cautionBackgroundSubMap,  bgGetMapPtr(bg[1]), cautionBackgroundSubMapLen);

    // can only write to extended palettes in LCD mode
	vramSetBankE(VRAM_E_LCD); // for main engine
    vramSetBankH(VRAM_H_LCD); // for subv engine

	// copy palettes to extended palette area
	dmaCopy(cautionBackgroundPal, &VRAM_E_EXT_PALETTE[2][0], cautionBackgroundPalLen);  // bg 2, slot 0 
    dmaCopy(cautionBackgroundSubPal,  &VRAM_H_EXT_PALETTE[1][0], cautionBackgroundSubPalLen);

	// map vram to extended palette
	vramSetBankE(VRAM_E_BG_EXT_PALETTE);
    vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE);

	bgUpdate();

	// fade caution screens in
	for(int i = 0; i <= 16; i++) {
        setBrightness(3, -16 + i);
	
		// wait for duration amount of frames
		for (int frame = 0; frame <= 6; frame++) {
			swiWaitForVBlank();
		}
	}

    // wait for duration amount of frames
    for (int frame = 0; frame <= 90; frame++) {
        swiWaitForVBlank();
    }

    // fade caution screens out
	for(int i = 0; i <= 16; i++) {
        setBrightness(3, -i);
	
		// wait for duration amount of frames
		for (int frame = 0; frame <= 6; frame++) {
			swiWaitForVBlank();
		}
	}
}

ViewState DisclaimerView::Update() {
    // transition to IntroView
    return ViewState::INTRO;
}

void DisclaimerView::Cleanup() {
    // clear screen
    setBrightness(3, 0);
    consoleClear();

    // reset vram
    vramSetBankA(VRAM_A_LCD);
    vramSetBankC(VRAM_C_LCD);
    vramSetBankD(VRAM_D_LCD);
    vramSetBankE(VRAM_E_LCD);
    vramSetBankH(VRAM_H_LCD);

    // reset backgrounds
    dmaFillHalfWords(0, bgGetMapPtr(bg[1]), 2048);
    dmaFillHalfWords(0, bgGetMapPtr(bg[0]), 2048);
}