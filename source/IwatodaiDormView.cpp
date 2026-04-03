#include <nds.h>
#include <stdio.h>
#include "globals.h"
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

// sub screen
int bgAkihiko;
PrintConsole console;

// world
const float tileSize = 0.0625f;
const float worldOffsetX = 1.8125f;
const float worldOffsetZ = 2.0f;
const float characterRadius = 0.05f;

// movement and viewpoint
const float speed = 0.01f;
const float angleIncrement = 0.05f;
const float distance = 0.5f; 
const float lookAhead = 0.3f;

// translation
float angle = 0.0;
float translateX = 0.0;
float translateZ = 0.0;
float characterFacingAngle = 0.0f;

// texture ID
static int environmentTextureId;
static int characterTextureId;

// check collision
TileType isTileAt(float worldX, float worldZ) {
    int tileX = (int)((worldX + worldOffsetX) / tileSize);
    int tileZ = (int)((worldZ + worldOffsetZ) / tileSize);

    // default
    if (tileX < 0 || tileX >= MAP_WIDTH || tileZ < 0 || tileZ >= MAP_HEIGHT)
        return TileType::NO_COLLISION;

    // else use collision data
    return (TileType)collision_map[tileZ][tileX];
}

// check all 4 corners of the character's bounding box
int isTileWalkable(TileType tileType, float worldX, float worldZ) {
    return
        (isTileAt(worldX - characterRadius, worldZ - characterRadius) != tileType) &&
        (isTileAt(worldX + characterRadius, worldZ - characterRadius) != tileType) &&
        (isTileAt(worldX - characterRadius, worldZ + characterRadius) != tileType) &&
        (isTileAt(worldX + characterRadius, worldZ + characterRadius) != tileType);
}

void DrawEnvironmentModel() {
    // bind texture before drawing
    glBindTexture(GL_TEXTURE_2D, environmentTextureId);
    glCallList((u32*)environment_bin);
}

void DrawPlayerModel() {
    glBindTexture(GL_TEXTURE_2D, characterTextureId);
    glCallList((u32*)character_bin);
}

void CharacterController() {
    scanKeys();
    u32 keys = keysHeld();

    float forwardX = -sin(angle) * speed;
    float forwardZ = cos(angle) * speed;
    float rightX = cos(angle) * speed;
    float rightZ = sin(angle) * speed;

    if(keys & KEY_L) angle -= angleIncrement;
    if(keys & KEY_R) angle += angleIncrement;

    float deltaX = 0.0f;
    float deltaZ = 0.0f;

    if(keys & KEY_UP) {
        deltaX += forwardX;
        deltaZ += forwardZ;
    }
    if(keys & KEY_DOWN) {
        deltaX -= forwardX;
        deltaZ -= forwardZ;
    }
    if(keys & KEY_RIGHT) {
        deltaX -= rightX;
        deltaZ -= rightZ;
    }
    if(keys & KEY_LEFT) {
        deltaX += rightX;
        deltaZ += rightZ;
    }

    float nextX = translateX + deltaX;
    float nextZ = translateZ + deltaZ;

    // try full movement first
    if (isTileWalkable(TileType::COLLISION, nextX, nextZ)) {
        translateX = nextX;
        translateZ = nextZ;
    }
    // if blocked, try X only (slide along Z wall)
    else if (isTileWalkable(TileType::COLLISION, nextX, translateZ)) {
        translateX = nextX;
    }
    // if blocked, try Z only (slide along X wall)
    else if (isTileWalkable(TileType::COLLISION, translateX, nextZ)) {
        translateZ = nextZ;
    }

    // only update the angle if button is being pressed
    if (deltaX != 0.0f || deltaZ != 0.0f) {
        // return angle in radians and convert to degrees
        float angleRad = atan2(deltaX, deltaZ);  
        characterFacingAngle = angleRad * (180.0f / 3.14159265f);
    }

    float cameraX = translateX + (sin(angle) * distance);
    float cameraY = 0.6f;
    float cameraZ = translateZ - (cos(angle) * distance);

    // look further down the same path the camera is facing
    float targetX = translateX - (sin(angle) * lookAhead);
    float targetY = 0.1f;
    float targetZ = translateZ + (cos(angle) * lookAhead);

    gluLookAt(cameraX, cameraY, cameraZ,
              targetX, targetY, targetZ,
              0.0f, 1.0f, 0.0f);
}


void DialougeController() {
    int line = 0;    
    while (true) {
        consoleClear();
        switch(line) {
            case 0:
                iprintf("\x1b[12;16HWhat's up?");
                break;
            case 1:
                iprintf("\x1b[12;16HTime to go?");
                break;
            case 2:
                iprintf("\x1b[12;16HDon't slack off.");
                break;
            default:
                return;
        }

        scanKeys();
        u32 keys = keysHeld();
        if (keys & KEY_A) {
            line++;
        } else if (keys & KEY_B) {
            line--;
        } if (keys & KEY_START) {
            return;
        }

        // delay between input and display
        for (int i = 0; i < 6; i++) {
            swiWaitForVBlank();
        }
    }
}

void InteractionController(u32 inputKeys) {
    switch(isTileAt(translateX, translateZ)) {
        case TileType::NEXT_SCENE:
            iprintf("\x1b[12;0HNext scene zone");
            break;
        case TileType::PREV_SCENE:
            iprintf("\x1b[12;0HPrev scene zone");
            break;
        case TileType::CHARACTER_Akihiko:
            iprintf("\x1b[12;16HPress A to talk");
            bgShow(bgAkihiko);
            if (inputKeys & KEY_A) {
                // delay between input and controller
                for (int i = 0; i < 6; i++) {
                    swiWaitForVBlank();
                }
                DialougeController();
            }
            break;
        default:
            consoleClear();
            bgHide(bgAkihiko);
    }
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

    // set character initial position
    translateX = -1.3;
    translateZ = -0.8;
    angle = -1.6;
    characterFacingAngle = 91.67;
}

ViewState IwatodaiDormView::Update() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    bgUpdate();

    scanKeys();
    u32 keys = keysHeld();
    if(keys & KEY_START) return ViewState::MAIN_MENU;

    // control character
    CharacterController();

    // draw environment
    glPushMatrix();
        DrawEnvironmentModel();
    glPopMatrix(1);

    // draw character
    glPushMatrix();
        // move character
        glTranslatef(translateX, 0, translateZ);
        glRotatef(characterFacingAngle, 0.0f, 1.0f, 0.0f);
        DrawPlayerModel();
    glPopMatrix(1);

    glFlush(0);

    InteractionController(keys);

    // print coordinates (64x64 area from 0,0 to 64,64)
    iprintf("\x1b[10;0Htile(x,z): %d, %d",
        (int)((translateX + worldOffsetX) / tileSize),
        (int)((translateZ + worldOffsetZ) / tileSize));
    iprintf("\x1b[11;0Htranslate(x,z): %d, %d",
        (int)(translateX * 100),
        (int)(translateZ * 100));
    iprintf("\x1b[12;0Hangle(w,c): %d, %d", (int)(angle * 100), (int)(characterFacingAngle * 100));
    return ViewState::KEEP_CURRENT;
}

void IwatodaiDormView::Cleanup() {
    // clear screen
    setBrightness(3, 0);
    consoleClear();

    // reset vram
    vramSetBankA(VRAM_A_LCD);
    vramSetBankB(VRAM_B_LCD);
    vramSetBankC(VRAM_C_LCD);
    vramSetBankH(VRAM_H_LCD);

    // reset backgrounds
    dmaFillHalfWords(0, bgGetMapPtr(bgAkihiko), 2048);

    // reset textures
    glDeleteTextures(1, &environmentTextureId);
    glDeleteTextures(1, &characterTextureId);
}