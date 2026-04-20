#include <nds.h>
#include <stdio.h>
#include "core/globals.h"
#include "IntroView.h"

// assets
#include "skyBackground.h"
#include "roomBackground.h"
#include "silhouetteBackground.h"
#include "overlayBackground.h"
#include "logoSpriteLeft.h"
#include "logoSpriteRight.h"
// sub screen
#include "attributionBackground.h"
#include "skyBackgroundSub.h"

// sub screen
int bgSubLogo;
int bgSubSky;
PrintConsole consoleIntro;

void IntroView::Init() {
    // set video mode for 3 text layers and 1 extended rotation layer
	videoSetMode(MODE_3_2D);
	// set sub video mode for 4 text layers
	videoSetModeSub(MODE_0_2D);

	// map vram bank A and D to main engine background (slot 0)
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	vramSetBankD(VRAM_D_MAIN_BG_0x06020000);
    // map vram bank B to sub engine background
    vramSetBankC(VRAM_C_SUB_BG);
	// map vram bank B to main engine sprites (slot 0)
	vramSetBankB(VRAM_B_MAIN_SPRITE);

	// enable extended palettes
	bgExtPaletteEnable();
    bgExtPaletteEnableSub();

	// set brightness on both screen to completely dark (no visible image)
	setBrightness(3, -16);

	// initialize backgrounds
	// check https://mtheall.com/vram.html to ensure bg fit in vram
	bg[0] = bgInit(0, BgType_Text8bpp, BgSize_T_512x512, 11, 2);		// silhouette
	bg[1] = bgInit(1, BgType_Text8bpp, BgSize_T_256x256, 9, 0);			// room
	bg[2] = bgInit(2, BgType_Text8bpp, BgSize_T_256x256, 10, 3);		// sky
	bg[3] = bgInit(3, BgType_ExRotation, BgSize_ER_512x512, 19, 8); 	// overlay
    bgSubLogo = bgInitSub(0, BgType_Text8bpp, BgSize_T_256x256, 0, 1);  // logo
    bgSubSky = bgInitSub(1, BgType_Text8bpp, BgSize_T_256x256, 1, 2);   // sky (sub screen)

    // setup console
    consoleInit(&consoleIntro, 2, BgType_Text4bpp, BgSize_T_256x256, 4, 5, false, true);
    consoleSelect(&consoleIntro);

	// need to set priority to properly display
	// 0 is highest, 3 is lowest
    bgSetPriority(bg[0], 1);	// silhouette
    bgSetPriority(bg[1], 1);	// room
    bgSetPriority(bg[2], 3);	// sky
	bgSetPriority(bg[3], 2);	// overlay
    // adjust sub screen image and console to sit correctly on each other
    bgSetPriority(consoleIntro.bgId, 0);
    bgSetPriority(bgSubLogo, 1);
    bgSetPriority(bgSubSky, 2);

    // reset background vram
    // 512x512 backgrounds use 8192 bytes of map memory
    // calculated using (512 / 8) * (512 / 8) * 2
    // the DS divides pixels into 8x8 tiles (hence we divide by 8) and we use 16-bit colour (which is 2 bytes)
    dmaFillHalfWords(0, bgGetMapPtr(bg[0]), 8192);  // silhouette
    dmaFillHalfWords(0, bgGetMapPtr(bg[3]), 8192);  // overlay
    // 256x256 backgrounds use 2048 bytes of map memory
    dmaFillHalfWords(0, bgGetMapPtr(bg[1]), 2048);  // room
    dmaFillHalfWords(0, bgGetMapPtr(bg[2]), 2048);  // sky
    dmaFillHalfWords(0, bgGetMapPtr(bgSubLogo), 2048);
    dmaFillHalfWords(0, bgGetMapPtr(bgSubSky), 2048);

	// copy graphics to vram
	dmaCopy(silhouetteBackgroundTiles,  bgGetGfxPtr(bg[0]), silhouetteBackgroundTilesLen);
	dmaCopy(roomBackgroundTiles,  bgGetGfxPtr(bg[1]), roomBackgroundTilesLen);
  	dmaCopy(skyBackgroundTiles, bgGetGfxPtr(bg[2]), skyBackgroundTilesLen);
	dmaCopy(overlayBackgroundTiles, bgGetGfxPtr(bg[3]), overlayBackgroundTilesLen);
    dmaCopy(attributionBackgroundTiles,  bgGetGfxPtr(bgSubLogo), attributionBackgroundTilesLen);
    dmaCopy(skyBackgroundSubTiles,  bgGetGfxPtr(bgSubSky), skyBackgroundSubTilesLen);

	// copy maps to vram
	dmaCopy(silhouetteBackgroundMap,  bgGetMapPtr(bg[0]), silhouetteBackgroundMapLen);
	dmaCopy(roomBackgroundMap,  bgGetMapPtr(bg[1]), roomBackgroundMapLen);
  	dmaCopy(skyBackgroundMap, bgGetMapPtr(bg[2]), skyBackgroundMapLen);
	dmaCopy(overlayBackgroundMap,   bgGetMapPtr(bg[3]), overlayBackgroundMapLen);
    dmaCopy(attributionBackgroundMap,  bgGetMapPtr(bgSubLogo), attributionBackgroundMapLen);
    dmaCopy(skyBackgroundSubMap,  bgGetMapPtr(bgSubSky), skyBackgroundSubMapLen);

    // can only write to extended palettes in LCD mode
	vramSetBankE(VRAM_E_LCD); // for main engine
    vramSetBankH(VRAM_H_LCD); // for subv engine

	// copy palettes to extended palette area
	dmaCopy(silhouetteBackgroundPal, &VRAM_E_EXT_PALETTE[0][0], silhouetteBackgroundPalLen);	// bg 0, slot 0 (slot can be specified slot in .grit file)
	dmaCopy(roomBackgroundPal,  &VRAM_E_EXT_PALETTE[1][0],  roomBackgroundPalLen);  			// bg 1, slot 0
	dmaCopy(skyBackgroundPal, &VRAM_E_EXT_PALETTE[2][0], skyBackgroundPalLen); 					// bg 2, slot 0 
	dmaCopy(overlayBackgroundPal,   &VRAM_E_EXT_PALETTE[3][0], overlayBackgroundPalLen);		// bg 3, slot 0
    dmaCopy(attributionBackgroundPal,  &VRAM_H_EXT_PALETTE[0][0], attributionBackgroundPalLen);
    dmaCopy(skyBackgroundSubPal,  &VRAM_H_EXT_PALETTE[1][0], skyBackgroundSubPalLen);

	// map vram to extended palette
	vramSetBankE(VRAM_E_BG_EXT_PALETTE);
    vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE);

	bgHide(bg[3]);	// hide overlay
	bgSetCenter(bg[3], 128, 96);	// pivot point on the screen (at the screen's center)
	bgSetScroll(bg[3], 256, 256);	// pivot point on the image (at the image's center)

	// showing logo as sprite
	logoSprite[0] = {0, SpriteSize_64x64, SpriteColorFormat_256Color, 0, 15, -25, 100};
    logoSprite[1] = {0, SpriteSize_64x64, SpriteColorFormat_256Color, 0, 15, 39, 100};
    // logoSprite[1] = {0, SpriteSize_64x64, SpriteColorFormat_256Color, 0, 15, 120, 100};

	// initialize sub sprite engine with 1D mapping, 128 byte boundry, no external palette support
	oamInit(&oamMain, SpriteMapping_1D_128, false);
	
	// allocating space for sprite graphics
	logoSprite[0].gfx = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_256Color);
    logoSprite[1].gfx = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_256Color);

	dmaCopy(logoSpriteLeftTiles, logoSprite[0].gfx, logoSpriteLeftTilesLen);
    dmaCopy(logoSpriteRightTiles, logoSprite[1].gfx, logoSpriteRightTilesLen);

    dmaCopy(logoSpriteLeftPal, SPRITE_PALETTE, logoSpriteLeftPalLen);
    dmaCopy(logoSpriteRightPal, SPRITE_PALETTE, logoSpriteRightPalLen);

	// for slide in animation
	// move camera to the empty right half of the 512px wide background
	bgSetScroll(bg[0], -silhouetteX, -silhouetteY);
	bgUpdate();

    // point to music
    musicCtrl.init("nitro:/music/tightrope.mp3", 17.962f, 66.082f);

    // hide sub screen text and attribution text layer
    REG_BLDCNT_SUB = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_SRC_BG0 | BLEND_DST_BG0 | BLEND_DST_BG1 | BLEND_DST_BACKDROP;
    REG_BLDALPHA_SUB = 0 | (16 << 8);

    // hide main skyBackground
	// blend control. takes effect mode / source / destination
	REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_DST_BACKDROP;

    // fade top screen in
    for(int i = 0; i <= 16; i++) {
        setBrightness(1, -16 + i);
	
		// wait for duration amount of frames
		for (int frame = 0; frame <= 3; frame++) {
            musicCtrl.update();
			swiWaitForVBlank();
		}
	}

    // fade skyBackgrounds in
	for(int i = 0; i <= 16; i++) {
		// source opacity / dest opacity. They should add up to 16
		REG_BLDALPHA = i | ((16 - i) << 8);
        // fade sub screen
        setBrightness(2, -16 + i);
	
		// wait for duration amount of frames
		for (int frame = 0; frame <= 6; frame++) {
            musicCtrl.update();
			swiWaitForVBlank();
		}
	}
}

ViewState IntroView::Update() {
    musicCtrl.update();
    scanKeys();
    int keys = keysDown();

    // transition to menu state on any input
    if (keys) {
        musicCtrl.pause();
        // transition both screens to white
        for(int i = 0; i <= 16; i++) {
            setBrightness(3, i);

            // wait a few frames
            for (int duration = 0; duration <= 2; duration++) {
                swiWaitForVBlank();
            }
        }
        return ViewState::MAIN_MENU;
    }
    
    touchRead(&touchXY);

    // scroll silhouette background
    // animate X (moving right towards 0)
    if (silhouetteX < 0 && frame % 5 == 0) {
        silhouetteX += (-silhouetteX) / 6 + 1;
        if (silhouetteX > 0) silhouetteX = 0;
    }

    // animate Y (moving up towards 0)
    if (silhouetteY > 0 && frame % 5 == 0) {
        silhouetteY += (-silhouetteY) / 6 + 1; 
        if (silhouetteY < 0) silhouetteY = 0;
    }

    bgSetScroll(bg[0], -silhouetteX, -silhouetteY);
    
    // perform code after silhouette slide-in
    if (silhouetteX < 0 || silhouetteY < 0) {
        return ViewState::KEEP_CURRENT;
    }

    if (animateText) {
        // NOTE: The text uses ansi escape sequences.
        // The bottom screen has 24 lines, 32 columns (from 0 -> 23, 0 -> 32)
        // Center the text by doing (32 / 2) - (len / 2)
        iprintf("\x1b[11;8HPress Any Button");

        durationCounter++;

        if (durationCounter >= duration) {
            durationCounter = 0;
            textAlpha += textAlphaDirection;
            
            if (textAlpha >= 16) {
                textAlpha = 16;
                textAlphaDirection = -1;    // Start fading out
            } else if (textAlpha <= 0) {
                textAlpha = 0;
                textAlphaDirection = 1;     // Start fading in
            }

            REG_BLDCNT_SUB = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_DST_BG0 | BLEND_DST_BG1 | BLEND_DST_BACKDROP;
            REG_BLDALPHA_SUB = textAlpha | ((16 - textAlpha) << 8);
        }
    }

    // setup logoSprite
    if (!displayLogo) {
        displayLogo = true;

        for (int i = 0; i < 2; i++) {
            oamSet(
                &oamMain, 						// main display (OamState)
                i,       						// oam entry to set (id)
                logoSprite[i].x, logoSprite[i].y, 	// position
                0, 								// priority
                logoSprite[i].paletteAlpha, 		// palette for 16 color sprite or alpha for bmp sprite
                logoSprite[i].size,
                logoSprite[i].format,
                logoSprite[i].gfx,
                logoSprite[i].rotationIndex,
                true, 							// double the size of rotated sprites
                false, 							// don't hide the sprite
                false, false, 					// vflip, hflip
                false 							// apply mosaic
                );

                oamMain.oamMemory[i].attribute[0] |= ATTR0_TYPE_BLENDED;
        }
        
        // setup fade for main screen sprites
        REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_SPRITE | 
                     BLEND_DST_BG0 | BLEND_DST_BG1 | BLEND_DST_BG2;
        REG_BLDALPHA = 0 | (16  << 8);

        // setup fade for sub screen attribution text layer
        REG_BLDCNT_SUB = BLEND_ALPHA | BLEND_SRC_BG0 | 
                         BLEND_DST_BG1 | BLEND_DST_BACKDROP;
        REG_BLDALPHA_SUB = 0 | (16  << 8);
    }

    // fade in attribution text layer and logoSprite
    if (logoOpacity < 16 && frame % 4 == 0) {
        logoOpacity++;
        REG_BLDALPHA = logoOpacity | ((16 - logoOpacity) << 8);
        REG_BLDALPHA_SUB = logoOpacity | ((16 - logoOpacity) << 8);
    }

    // code after sprite fade in
    if (logoOpacity < 16) {
        oamMain.oamMemory[0].attribute[0] &= ~ATTR0_TYPE_BLENDED;	// disable sprite blending
        oamMain.oamMemory[1].attribute[0] &= ~ATTR0_TYPE_BLENDED;	// disable sprite blending
        return ViewState::KEEP_CURRENT;
    }

    // setup animated text
    if (logoOpacity >= 16 && !animateText) {
        animateText = true;
        REG_BLDCNT_SUB = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_DST_BG0 | BLEND_DST_BG1 | BLEND_DST_BACKDROP;
        REG_BLDALPHA_SUB = textAlpha | ((16 - textAlpha) << 8);
    }

    // setup blending for overlay
    if (!displayOverlay) {
        displayOverlay = true;
        REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG3 | BLEND_DST_BG2;
        bgShow(bg[3]);
    }

    // fade in overlay
    if (overlayOpacity < 6 && frame % 4 == 0) {
        overlayOpacity++;
        REG_BLDALPHA = overlayOpacity | ((16 - overlayOpacity) << 8);
    }

    // rotate overlay
    if (frame % 4 == 0) {
        waveAngle += 50;
        // NOTE: since DS does not have floating point numbers, sinLerp returns value from -4096 -> 4096, which is why we divide by 4096 (shift >> 12)
        int rotationSpeed = baseSpeed + ((sinLerp(waveAngle) * fluctuation) >> 12);
        currentRotation += rotationSpeed;
        bgSetRotateScale(bg[3], currentRotation, 256, 256);
    }

    // default state
    return ViewState::KEEP_CURRENT;
}

void IntroView::Cleanup() {
    // clear screen
    setBrightness(3, 0);
    consoleClear();

    // reset vram
    vramSetBankA(VRAM_A_LCD);
    vramSetBankB(VRAM_B_LCD);
    vramSetBankC(VRAM_C_LCD);
    vramSetBankD(VRAM_D_LCD);
    vramSetBankE(VRAM_E_LCD);
    vramSetBankH(VRAM_H_LCD);

    // reset backgrounds
    dmaFillHalfWords(0, bgGetMapPtr(bg[0]), 8192);  // silhouette
    dmaFillHalfWords(0, bgGetMapPtr(bg[3]), 8192);  // overlay
    // 256x256 backgrounds use 2048 bytes of map memory
    dmaFillHalfWords(0, bgGetMapPtr(bg[1]), 2048);  // room
    dmaFillHalfWords(0, bgGetMapPtr(bg[2]), 2048);  // sky

    // clear all sprites from oam
    oamClear(&oamMain, 0, 0);

    // free allocated sprite vram
    for (int i = 0; i < 2; i++) {
        if (logoSprite[i].gfx != NULL) {
            oamFreeGfx(&oamMain, logoSprite[i].gfx);
        }
    }

    // disable blending
    REG_BLDCNT = 0;
    REG_BLDCNT_SUB = 0;
    REG_BLDALPHA = 0;
    REG_BLDALPHA_SUB = 0;
}