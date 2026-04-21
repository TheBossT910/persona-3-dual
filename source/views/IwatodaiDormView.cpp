#include <nds.h>
#include <stdio.h>
#include "core/globals.h"
#include "math.h"
#include "IwatodaiDormView.h"

// assets
// 3D models
#include "environment_bin.h"
#include "character_bin.h"
// textures
#include "character.h"
#include "environment.h"
// collision
#include "IwatodaiDormCollision.h"
// 2D
#include "akihiko.h"

// texture ID
static int environmentTextureId;
static int characterTextureId;

void DrawEnvironmentModel() {
    // bind texture before drawing
    glBindTexture(GL_TEXTURE_2D, environmentTextureId);
    glCallList((u32*)environment_bin);
}

void DrawPlayerModel() {
    // bind texture before drawing
    glBindTexture(GL_TEXTURE_2D, characterTextureId);
    glCallList((u32*)character_bin);
}

void IwatodaiDormView::Init() {
    videoSetMode(MODE_0_3D);
    videoSetModeSub(MODE_0_2D);

    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankB(VRAM_B_TEXTURE);
    vramSetBankC(VRAM_C_SUB_BG);
    bgExtPaletteEnableSub();

    // main screen, 3D
    glInit();
    glEnable(GL_ANTIALIAS);
    glEnable(GL_TEXTURE_2D);    // enable texturing

    glClearColor(0,0,0,31);
    glClearPolyID(63);
    glClearDepth(0x7FFF);

    // set size of main screen
    glViewport(0,0,255,191);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // zNear is how close the camera can see, zFar is the maximum draw distance
    gluPerspective(55, 256.0 / 192.0, 0.1, 40);    

    // environment
    glGenTextures(1, &environmentTextureId);
    glBindTexture(GL_TEXTURE_2D, environmentTextureId);
    glTexImage2D(
        GL_TEXTURE_2D, 0,
        GL_RGBA,
        TEXTURE_SIZE_256, TEXTURE_SIZE_256,
        0,
        TEXGEN_TEXCOORD | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T,
        environmentBitmap  // from environment.h
    );

    // character
    glGenTextures(1, &characterTextureId);
    glBindTexture(GL_TEXTURE_2D, characterTextureId);
    glTexImage2D(
        GL_TEXTURE_2D, 0,
        GL_RGBA,
        TEXTURE_SIZE_16, TEXTURE_SIZE_16,
        0,
        TEXGEN_TEXCOORD | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T,
        characterBitmap  // from character.h
    );

    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
    glColor3b(255, 255, 255);   // keep white so texture colors aren't tinted

    // sub screen, 2D and console
    bgAkihiko = bgInitSub(0, BgType_Text8bpp, BgSize_T_256x256, 0, 1);
    dmaFillHalfWords(0, bgGetMapPtr(bgAkihiko), 2048);
    // hiding Akihiko by default
    bgHide(bgAkihiko);

    dmaCopy(akihikoTiles,  bgGetGfxPtr(bgAkihiko), akihikoTilesLen);
    dmaCopy(akihikoMap,  bgGetMapPtr(bgAkihiko), akihikoMapLen);
    vramSetBankH(VRAM_H_LCD);                   // can only write to extended palettes in LCD mode
    dmaCopy(akihikoPal,  &VRAM_H_EXT_PALETTE[0][0], akihikoPalLen);
    vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE);    // map vram banks to extended palettes
    
    // setup console
    consoleInit(&console, 1, BgType_Text4bpp, BgSize_T_256x256, 4, 5, false, true);
    consoleSelect(&console);

    // adjust sub screen image and console to sit correctly on each other
    bgSetPriority(console.bgId, 0);
    bgSetPriority(bgAkihiko, 1);

    bgUpdate();

    // get controllers
    playerCtrl = new CharacterController(MAP_WIDTH, MAP_HEIGHT, &collision_map[0][0], tileSize, worldOffsetX, worldOffsetZ, characterRadius, speed, angleIncrement, distance, lookAhead, angle, translateX, translateZ, characterFacingAngle);
    
    // build dialogue graph for dialogueCtrl
    lines[0] = { "Akihiko", "text line 1", bgAkihiko, NULL, &lines[1], {} };
    lines[1] = { "Akihiko", "text line 2", bgAkihiko, &lines[0], &lines[2], {} };

    lines[3] = { "Akihiko", "selection 1", bgAkihiko, &lines[2], NULL, {} };
    lines[4] = { "Akihiko", "selection 2", bgAkihiko, &lines[2], NULL, {} };

    dialogueSelection sel1 = { "Go home",         false, &lines[3] };
    dialogueSelection sel2 = { "Go for a walk",   false, &lines[4] };
    
    lines[2] = { "Akihiko", "line 3, sel", bgAkihiko, &lines[1], NULL, { sel1, sel2 } };

    // point to music
    musicCtrl.init("nitro:/music/changing_seasons.pcm", 0.0f, -1.0f);
}

ViewState IwatodaiDormView::Update() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    bgUpdate();

    scanKeys();
    u32 keys = keysHeld();
    u32 pressed = keysDown();
    if(keys & KEY_START) return ViewState::MAIN_MENU;

    // only process world input when dialogue is not active
    if (!dialogueCtrl.isActive()) {
        // move character
        camPos = playerCtrl->update(keys);

        // trigger dialogue from interaction
        if (playerCtrl->isTileAt() == TileType::CHARACTER_Akihiko) {
            iprintf("\x1b[0;0HPress A to talk");
            if (pressed & KEY_A) {
                dialogueCtrl.start(&lines[0]);
            }
        } else {
            consoleClear();
        }
    }

    // update camera position
    gluLookAt(camPos.cameraX, camPos.cameraY, camPos.cameraZ,
        camPos.targetX, camPos.targetY, camPos.targetZ,
        camPos.upX, camPos.upY, camPos.upZ);

    // draw environment
    glPushMatrix();
        DrawEnvironmentModel();
    glPopMatrix(1);

    // draw character
    glPushMatrix();
        // move character
        characterPosition charPos = playerCtrl->isCharacterAt();
        glTranslatef(charPos.x, 0, charPos.z);
        glRotatef(charPos.facingAngle, 0.0f, 1.0f, 0.0f);
        DrawPlayerModel();
    glPopMatrix(1);

    glFlush(0);

    // print coordinates (64x64 area from 0,0 to 64,64)
    iprintf("\x1b[21;0Htile(x,z): %d, %d",
        (int)((charPos.x + worldOffsetX) / tileSize),
        (int)((charPos.z + worldOffsetZ) / tileSize));
    iprintf("\x1b[22;0Htranslate(x,z): %d, %d",
        (int)(charPos.x * 100),
        (int)(charPos.z * 100));
    iprintf("\x1b[23;0Hangle(w,c): %d, %d", (int)(charPos.angle * 100), (int)(charPos.facingAngle * 100));

    // update controllers
    dialogueCtrl.update(keys);
    musicCtrl.update();

    return ViewState::KEEP_CURRENT;
}

void IwatodaiDormView::Cleanup() {
    // clear screen
    setBrightness(3, 0);
    consoleClear();

    // reset backgrounds
    dmaFillHalfWords(0, bgGetMapPtr(bgAkihiko), 2048);

    // reset textures
    glDeleteTextures(1, &environmentTextureId);
    glDeleteTextures(1, &characterTextureId);

    // reset vram
    vramSetBankA(VRAM_A_LCD);
    vramSetBankB(VRAM_B_LCD);
    vramSetBankC(VRAM_C_LCD);
    vramSetBankH(VRAM_H_LCD);

    // cleanup controllers
    delete playerCtrl;
    playerCtrl = NULL;
}