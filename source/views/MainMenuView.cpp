#include <nds.h>
#include <stdio.h>
#include "core/globals.h"
#include "MainMenuView.h"

// assets
#include "menuSilhouetteBackground.h"
#include "doorBackground.h"
#include "fogBackground.h"
// sfx
#include "soundbank.h"

mm_sfxhand sfxMenuHandle;
mm_sfxhand sfxSelectHandle;
mm_sfxhand sfxCancelHandle;

void cancelSFX() {
    musicCtrl.stopSFX(sfxMenuHandle);
    musicCtrl.stopSFX(sfxSelectHandle);
    musicCtrl.stopSFX(sfxCancelHandle);
}

void MainMenuView::Init() {
    // point to music
    musicCtrl.loadSFX(SFX_MENU);
    musicCtrl.loadSFX(SFX_SELECT);
    musicCtrl.loadSFX(SFX_CANCEL);
    musicCtrl.init("nitro:/music/aria_of_the_soul.pcm", 0.0f, 164.940f);

    // transition both screens from white
    for(int i = 16; i > 0; i--) {
        setBrightness(3, i);
    
        // wait a few frames
        for (int duration = 0; duration <= 2; duration++) {
            swiWaitForVBlank();
            musicCtrl.update();
        }
    }

    // set video mode for 2 text layers and 2 extended rotation layer
	videoSetMode(MODE_5_2D);
	// set sub video mode for 4 text layers
	videoSetModeSub(MODE_0_2D);

	// map vram bank A to main engine background (slot 0)
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	vramSetBankD(VRAM_D_MAIN_BG_0x06020000);
	// map vram bank B to main engine sprites (slot 0)
	vramSetBankB(VRAM_B_MAIN_SPRITE);

	// enable extended palettes
	bgExtPaletteEnable();

	// debug init
	// NOTE: for some reason, we cant use vram bank C. It might be because of consoleDemoInit...
	consoleDemoInit();

	// set brightness on bottom screen to completely dark (no visible image)
	setBrightness(2, -16);

	// initialize backgrounds
	// check https://mtheall.com/vram.html to ensure bg fit in vram
	bg[0] = bgInit(0, BgType_Text8bpp, BgSize_T_512x512, 4, 0);		// silhouette
    bg[1] = bgInit(1, BgType_Text8bpp, BgSize_T_256x256, 9, 2);		// door
    bg[2] = bgInit(2, BgType_ExRotation, BgSize_ER_256x256, 8, 5); 	// fog
	
	// need to set priority to properly display
	// 0 is highest, 3 is lowest
    bgSetPriority(bg[0], 0);	// silhouette
    bgSetPriority(bg[2], 1);	// fog
    bgSetPriority(bg[1], 2);	// door

    // reset background vram
    dmaFillHalfWords(0, bgGetMapPtr(bg[0]), 8192);
    dmaFillHalfWords(1, bgGetMapPtr(bg[0]), 2048);
    dmaFillHalfWords(2, bgGetMapPtr(bg[0]), 2048);
	
	// copy graphics to vram
	dmaCopy(menuSilhouetteBackgroundTiles,  bgGetGfxPtr(bg[0]), menuSilhouetteBackgroundTilesLen);
    dmaCopy(doorBackgroundTiles,  bgGetGfxPtr(bg[1]), doorBackgroundTilesLen);
    dmaCopy(fogBackgroundTiles, bgGetGfxPtr(bg[2]), fogBackgroundTilesLen);

	// copy maps to vram
	dmaCopy(menuSilhouetteBackgroundMap,  bgGetMapPtr(bg[0]), menuSilhouetteBackgroundMapLen);
    dmaCopy(doorBackgroundMap,  bgGetMapPtr(bg[1]), doorBackgroundMapLen);
    dmaCopy(fogBackgroundMap,   bgGetMapPtr(bg[2]), fogBackgroundMapLen);

	vramSetBankE(VRAM_E_LCD); // for main engine

	// copy palettes to extended palette area
	dmaCopy(menuSilhouetteBackgroundPal, &VRAM_E_EXT_PALETTE[0][0], menuSilhouetteBackgroundPalLen);
    dmaCopy(doorBackgroundPal,  &VRAM_E_EXT_PALETTE[1][0],  doorBackgroundPalLen);
    dmaCopy(fogBackgroundPal,   &VRAM_E_EXT_PALETTE[2][0], fogBackgroundPalLen);

	// map vram to extended palette
	vramSetBankE(VRAM_E_BG_EXT_PALETTE);

    bgHide(bg[2]);
	bgSetCenter(bg[2], 128, 96);	// pivot point on the screen (at the screen's center)
	bgSetScroll(bg[2], 128, 96);	// pivot point on the image (at the image's center)

	// for slide in animation
	// move camera to the empty right half of the 512px wide background
	bgSetScroll(bg[0], -silhouetteX, -silhouetteY);
	bgUpdate();

    // fade door background in
    REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG1 | BLEND_DST_BACKDROP;
	for(int i = 0; i <= 16; i++) {
		REG_BLDALPHA = i | ((16 - i) << 8);
	
		// wait for duration amount of frames
		for (int frame = 0; frame <= 6; frame++) {
			swiWaitForVBlank();
            musicCtrl.update();
		}
	}

    // set default options to menuOptions
    options = menuOptions;
    optionCount = menuOptionCount;
}

ViewState MainMenuView::Update() {
    scanKeys();
    int keys = keysDown();

    // display option text
    if (keys & KEY_DOWN) {
        sfxMenuHandle = musicCtrl.playSFX(SFX_MENU, 255, 128);
        selectedOption = (selectedOption + 1) % optionCount;
    }

    if (keys & KEY_UP) {
        sfxMenuHandle = musicCtrl.playSFX(SFX_MENU, 255, 128);
        selectedOption = (selectedOption + optionCount - 1) % optionCount;
    }

    if (keys & KEY_A) {
        sfxSelectHandle = musicCtrl.playSFX(SFX_SELECT, 255, 128);
        options[selectedOption].selected = true;
    }

    if (keys & KEY_B) {
        sfxCancelHandle = musicCtrl.playSFX(SFX_CANCEL, 255, 128);
        options[selectedOption].selected = false;
    }

    consoleClear();
    // choosing which options to display
    if (menuOptions[0].selected) {
        cancelSFX();
        musicCtrl.playSFX(SFX_SELECT, 255, 128);

        // select sceneOptions
        menuOptions[0].selected = false;
        selectedOption = 0;
        options = sceneOptions;
        optionCount = sceneOptionCount;
    } else if (menuOptions[1].selected) {
        cancelSFX();
        musicCtrl.playSFX(SFX_CANCEL, 255, 128);
        musicCtrl.pause();

        // selected "Return to Title"
        for(int i = 0; i > -16; i--) {
            setBrightness(3, i);
            for (int duration = 0; duration <= 2; duration++) {
                musicCtrl.update();
                swiWaitForVBlank();
            }
        }
        return ViewState::INTRO;
    } else if (sceneOptions[0].selected) {
        cancelSFX();
        musicCtrl.playSFX(SFX_SELECT, 255, 128);
        musicCtrl.pause();
        
        // selected "Iwatodai Dorm"
        for(int i = 0; i > -16; i--) {
            setBrightness(3, i);
            for (int duration = 0; duration <= 2; duration++) {
                musicCtrl.update();
                swiWaitForVBlank();
            }
        }
        return ViewState::IWATODAI_DORM;
    } else if (sceneOptions[1].selected) {
        cancelSFX();
        musicCtrl.playSFX(SFX_CANCEL, 255, 128);

        // select menuOptions
        sceneOptions[1].selected = false;
        selectedOption = 0;
        options = menuOptions;
        optionCount = menuOptionCount;
    }

    for (int option = 0; option < optionCount; option++) {
        // display options
        iprintf("%c %s\n", option == selectedOption ? '>' : ' ', options[option].name);
    }

    musicCtrl.update();

    // scroll silhouette background
    // animate X (moving right towards 0)
    if (silhouetteX < 0 && frame % 5 == 0) {
        silhouetteX += (-silhouetteX) / 6 + 1;
        if (silhouetteX > 0) silhouetteX = 0;
    }

    // animate Y (moving up towards 0)
    if (silhouetteY > 0 && frame % 5 == 0) {
        silhouetteY -= (silhouetteY / 6) + 1; 
        if (silhouetteY < 0) silhouetteY = 0;
    }

    bgSetScroll(bg[0], -silhouetteX, -silhouetteY);
    
    // perform code after silhouette slide-in
    if (silhouetteX < 0 || silhouetteY < 0) {
        return ViewState::KEEP_CURRENT;
    }

    // fade in bottom screen text
    if (brightness < 16 && frame % 4 == 0) {
        brightness++;
        setBrightness(2, brightness - 16);
    }

    // setup blending for fog
    if (!displayFog) {
        displayFog = true;
        REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_DST_BACKDROP | BLEND_DST_BG1;
        bgShow(bg[2]);
    }

    // fade in fog
    if (displayFog && fogOpacity < 6 && frame % 4 == 0) {
        fogOpacity++;
        REG_BLDALPHA = fogOpacity | ((16 - fogOpacity) << 8);
    }

    // rotate fog
    if (displayFog && frame % 4 == 0) {
        waveAngle += 50;        
        int rotationSpeed = baseSpeed + ((sinLerp(waveAngle) * fluctuation) >> 12);
        currentRotation += rotationSpeed;
        bgSetRotateScale(bg[2], currentRotation, 256, 256);
    }

    // default state
    return ViewState::KEEP_CURRENT;
}

void MainMenuView::Cleanup() {
    // clear screen
    setBrightness(3, 0);
    consoleClear();

    // reset backgrounds
    dmaFillHalfWords(0, bgGetMapPtr(bg[0]), 8192);
    dmaFillHalfWords(1, bgGetMapPtr(bg[1]), 2048);
    dmaFillHalfWords(2, bgGetMapPtr(bg[2]), 2048);

    // reset vram
    vramSetBankA(VRAM_A_LCD);
    vramSetBankB(VRAM_B_LCD);
    vramSetBankD(VRAM_D_LCD);
    vramSetBankE(VRAM_E_LCD);

    // disable blending
    REG_BLDCNT = 0;
    REG_BLDALPHA = 0;
}