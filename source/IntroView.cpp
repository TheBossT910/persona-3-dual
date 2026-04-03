#include <nds.h>
#include <stdio.h>
#include "globals.h"
#include "IntroView.h"

// assets
#include "skyBackground.h"
#include "roomBackground.h"
#include "silhouetteBackground.h"
#include "overlayBackground.h"
#include "logoSprite.h"

void IntroView::Init() {
    // set video mode for 3 text layers and 1 extended rotation layer
	videoSetMode(MODE_3_2D);
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
	bg[0] = bgInit(0, BgType_Text8bpp, BgSize_T_512x512, 11, 2);		// silhouette
	bg[1] = bgInit(1, BgType_Text8bpp, BgSize_T_256x256, 9, 0);			// room
	bg[2] = bgInit(2, BgType_Text8bpp, BgSize_T_256x256, 10, 3);		// sky
	bg[3] = bgInit(3, BgType_ExRotation, BgSize_ER_512x512, 19, 8); 	// overlay

	// need to set priority to properly display
	// 0 is highest, 3 is lowest
    bgSetPriority(bg[0], 1);	// silhouette
    bgSetPriority(bg[1], 1);	// room
    bgSetPriority(bg[2], 3);	// sky
	bgSetPriority(bg[3], 2);	// overlay

    // reset background vram
    // 512x512 backgrounds use 8192 bytes of map memory
    // calculated using (512 / 8) * (512 / 8) * 2
    // the DS divides pixels into 8x8 tiles (hence we divide by 8) and we use 16-bit colour (which is 2 bytes)
    dmaFillHalfWords(0, bgGetMapPtr(bg[0]), 8192);  // silhouette
    dmaFillHalfWords(0, bgGetMapPtr(bg[3]), 8192);  // overlay
    // 256x256 backgrounds use 2048 bytes of map memory
    dmaFillHalfWords(0, bgGetMapPtr(bg[1]), 2048);  // room
    dmaFillHalfWords(0, bgGetMapPtr(bg[2]), 2048);  // sky

	// copy graphics to vram
	dmaCopy(silhouetteBackgroundTiles,  bgGetGfxPtr(bg[0]), silhouetteBackgroundTilesLen);
	dmaCopy(roomBackgroundTiles,  bgGetGfxPtr(bg[1]), roomBackgroundTilesLen);
  	dmaCopy(skyBackgroundTiles, bgGetGfxPtr(bg[2]), skyBackgroundTilesLen);
	dmaCopy(overlayBackgroundTiles, bgGetGfxPtr(bg[3]), overlayBackgroundTilesLen);

	// copy maps to vram
	dmaCopy(silhouetteBackgroundMap,  bgGetMapPtr(bg[0]), silhouetteBackgroundMapLen);
	dmaCopy(roomBackgroundMap,  bgGetMapPtr(bg[1]), roomBackgroundMapLen);
  	dmaCopy(skyBackgroundMap, bgGetMapPtr(bg[2]), skyBackgroundMapLen);
	dmaCopy(overlayBackgroundMap,   bgGetMapPtr(bg[3]), overlayBackgroundMapLen);

	vramSetBankE(VRAM_E_LCD); // for main engine

	// copy palettes to extended palette area
	dmaCopy(silhouetteBackgroundPal, &VRAM_E_EXT_PALETTE[0][0], silhouetteBackgroundPalLen);	// bg 0, slot 0 (slot can be specified slot in .grit file)
	dmaCopy(roomBackgroundPal,  &VRAM_E_EXT_PALETTE[1][0],  roomBackgroundPalLen);  			// bg 1, slot 0
	dmaCopy(skyBackgroundPal, &VRAM_E_EXT_PALETTE[2][0], skyBackgroundPalLen); 					// bg 2, slot 0 
	dmaCopy(overlayBackgroundPal,   &VRAM_E_EXT_PALETTE[3][0], overlayBackgroundPalLen);		// bg 3, slot 0

	// map vram to extended palette
	vramSetBankE(VRAM_E_BG_EXT_PALETTE);

	bgHide(bg[3]);	// hide overlay
	bgSetCenter(bg[3], 128, 96);	// pivot point on the screen (at the screen's center)
	bgSetScroll(bg[3], 256, 256);	// pivot point on the image (at the image's center)

	// showing logo as sprite
	logoSprite = {0, SpriteSize_64x64, SpriteColorFormat_256Color, 0, 15, 0, 120};

	// initialize sub sprite engine with 1D mapping, 128 byte boundry, no external palette support
	oamInit(&oamMain, SpriteMapping_1D_128, false);
	
	// allocating space for sprite graphics
	u16* logoSpriteGfxPtr = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_256Color);
	dmaCopy(logoSpriteTiles, logoSpriteGfxPtr, logoSpriteTilesLen);
    dmaCopy(logoSpritePal, SPRITE_PALETTE, logoSpritePalLen);
    logoSprite.gfx = logoSpriteGfxPtr;

	// text uses ansi escape sequences
	iprintf("Taha Rashid\n");
	iprintf("\033[31;1;4mFeb 18, 2025\n\x1b[39m");
	iprintf("Line 3\n");
	iprintf("\x1b[32;1mLine 4\n\x1b[39m");
	iprintf("\x1b[31;1;4mLine 5\n\x1b[39m");
	iprintf("Line 6\n");

	// NOTE: bottom screen has 24 lines, 32 columns (from 0 -> 23, 0 -> 32)
	iprintf("\x1b[23;31HTest!");
	// center the text by doing (32 / 2) - (len / 2)
	iprintf("\x1b[11;8HPress Any Button");

	// for slide in animation
	// move camera to the empty right half of the 512px wide background
	bgSetScroll(bg[0], -silhouetteX, -silhouetteY);
	bgUpdate();

	// fade skyBackground in
	// blend control. takes effect mode / source / destination
	REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_DST_BACKDROP;
	for(int i = 0; i <= 16; i++) {
		// source opacity / dest opacity. They should add up to 16
		REG_BLDALPHA = i | ((16 - i) << 8);
	
		// wait for duration amount of frames
		for (int frame = 0; frame <= 6; frame++) {
			swiWaitForVBlank();
		}
	}
}

ViewState IntroView::Update() {
    scanKeys();
    int keys = keysDown();

    // transition to menu state on any input
    if (keys) {
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

    // print at using ansi escape sequence \x1b[line;columnH 
    iprintf("\x1b[10;0HFrame = %d",frame);
    iprintf("\x1b[16;0HTouch x = %04X, %04X\n", touchXY.rawx, touchXY.px);
    iprintf("Touch y = %04X, %04X\n", touchXY.rawy, touchXY.py);

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

    // animate bottom screen text
    if (animateText) {
        durationCounter++;
        if (durationCounter >= duration) {
            durationCounter = 0;
            brightnessCounter++;
            setBrightness(2, brightnessCounter - 16);
        }

        if (brightnessCounter >= brightness) {
            brightnessCounter = 0;
        }
    }

    // setup logoSprite
    if (!displayLogo) {
        displayLogo = true;
        oamSet(
            &oamMain, 						// main display (OamState)
            0,       						// oam entry to set (id)
            logoSprite.x, logoSprite.y, 	// position
            0, 								// priority
            logoSprite.paletteAlpha, 		// palette for 16 color sprite or alpha for bmp sprite
            logoSprite.size,
            logoSprite.format,
            logoSprite.gfx,
            logoSprite.rotationIndex,
            true, 							// double the size of rotated sprites
            false, 							// don't hide the sprite
            false, false, 					// vflip, hflip
            false 							// apply mosaic
            );
        
        oamMain.oamMemory[0].attribute[0] |= ATTR0_TYPE_BLENDED;
        // source is sprite, dest is all bgs
        REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_SPRITE | 
                    BLEND_DST_BG0 | BLEND_DST_BG1 | BLEND_DST_BG2;
        REG_BLDALPHA = 0 | (16  << 8);
    }

    // fade in logoSprite
    if (logoOpacity < 16 && frame % 4 == 0) {
        logoOpacity++;
        REG_BLDALPHA = logoOpacity | ((16 - logoOpacity) << 8);
    }

    // code after sprite fade in
    if (logoOpacity < 16) {
        oamMain.oamMemory[0].attribute[0] &= ~ATTR0_TYPE_BLENDED;	// disable sprite blending
        return ViewState::KEEP_CURRENT;
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
    vramSetBankD(VRAM_D_LCD);
    vramSetBankE(VRAM_E_LCD);

    // reset backgrounds
    dmaFillHalfWords(0, bgGetMapPtr(bg[0]), 8192);  // silhouette
    dmaFillHalfWords(0, bgGetMapPtr(bg[3]), 8192);  // overlay
    // 256x256 backgrounds use 2048 bytes of map memory
    dmaFillHalfWords(0, bgGetMapPtr(bg[1]), 2048);  // room
    dmaFillHalfWords(0, bgGetMapPtr(bg[2]), 2048);  // sky

    // clear all sprites from oam
    oamClear(&oamMain, 0, 0);

    // free allocated sprite vram
    if (logoSprite.gfx != NULL) {
        oamFreeGfx(&oamMain, logoSprite.gfx);
    }

    // disable blending
    REG_BLDCNT = 0;
    REG_BLDALPHA = 0;
}