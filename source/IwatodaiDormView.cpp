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

    if(keys & KEY_L) angle += angleIncrement;
    if(keys & KEY_R) angle -= angleIncrement;

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

    gluLookAt(  cameraX, cameraY, cameraZ,
                targetX, targetY, targetZ,
                0.0f, 1.0f, 0.0f);
}

void InteractionController() {
    if(isTileAt(translateX, translateZ) == TileType::NEXT_SCENE) {
        iprintf("\x1b[12;0HNext scene zone");
    } else if(isTileAt(translateX, translateZ) == TileType::PREV_SCENE) {
        iprintf("\x1b[12;0HPrev scene zone");
    } else if(isTileAt(translateX, translateZ) == TileType::CHARACTER_Akihiko) {
        iprintf("\x1b[12;0HAkihiko zone");
    } else {
        iprintf("\x1b[2J");
    }
}

void IwatodaiDormView::Init() {
    videoSetMode(MODE_0_3D);
    videoSetModeSub(MODE_0_2D);

    glInit();
    glEnable(GL_ANTIALIAS);
    glEnable(GL_TEXTURE_2D);    // enable texturing

    glClearColor(0,0,0,31);
    glClearPolyID(63);
    glClearDepth(0x7FFF);

    // set size of screen
    glViewport(0,0,255,191);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // zNear is how close the camera can see, zFar is the maximum draw distance
    gluPerspective(55, 256.0 / 192.0, 0.1, 40);

    // load texture
    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankB(VRAM_B_TEXTURE);

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

    consoleDemoInit();
}

ViewState IwatodaiDormView::Update() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

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

    InteractionController();

    // print coordinates (64x64 area from 0,0 to 64,64)
    iprintf("\x1b[10;0Htile: %d, %d",
        (int)((translateX + worldOffsetX) / tileSize),
        (int)((translateZ + worldOffsetZ) / tileSize));

    return ViewState::KEEP_CURRENT;
}

void IwatodaiDormView::Cleanup() {
    setBrightness(3, 0);
    iprintf("\x1b[2J");
}