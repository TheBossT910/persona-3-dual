#include <nds.h>
#include <stdio.h>
#include "core/globals.h"
#include "math.h"
#include "CharacterController.h"

// check collision
TileType CharacterController::isTileAt(float worldX, float worldZ) {
    int tileX;
    int tileZ;

    tileX = (int)((worldX + worldOffsetX) / tileSize);
    tileZ = (int)((worldZ + worldOffsetZ) / tileSize);

    // default
    if (tileX < 0 || tileX >= mapWidth || tileZ < 0 || tileZ >= mapHeight)
        return TileType::NO_COLLISION;

    // else use collision data
    return (TileType)collisionMap[(tileZ * mapWidth) + tileX];
}

// check all 4 corners of the character's bounding box
int CharacterController::isTileWalkable(float worldX, float worldZ) {
    return
        (isTileAt(worldX - characterRadius, worldZ - characterRadius) != TileType::COLLISION) &&
        (isTileAt(worldX + characterRadius, worldZ - characterRadius) != TileType::COLLISION) &&
        (isTileAt(worldX - characterRadius, worldZ + characterRadius) != TileType::COLLISION) &&
        (isTileAt(worldX + characterRadius, worldZ + characterRadius) != TileType::COLLISION);
}

// get tile at character position
TileType CharacterController::isTileAt() {
    return isTileAt(translateX, translateZ);
}

// get character position
characterPosition CharacterController::isCharacterAt() {
    characterPosition charPos;

    charPos.x = translateX;
    charPos.z = translateZ;
    charPos.angle = angle;
    charPos.facingAngle = characterFacingAngle;

    return charPos;
}

cameraPosition CharacterController::update(u32 keys) {
    float forwardX;
    float forwardZ;
    float rightX;
    float rightZ;

    float deltaX = 0.0f;
    float deltaZ = 0.0f;

    float nextX;
    float nextZ;

    float angleRad;

    cameraPosition camPos;

    forwardX = -sin(angle) * speed;
    forwardZ = cos(angle) * speed;
    rightX = cos(angle) * speed;
    rightZ = sin(angle) * speed;

    if(keys & KEY_L) angle -= angleIncrement;
    if(keys & KEY_R) angle += angleIncrement;

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

    nextX = translateX + deltaX;
    nextZ = translateZ + deltaZ;

    // try full movement first
    if (isTileWalkable(nextX, nextZ)) {
        translateX = nextX;
        translateZ = nextZ;
    }
    // if blocked, try X only (slide along Z wall)
    else if (isTileWalkable(nextX, translateZ)) {
        translateX = nextX;
    }
    // if blocked, try Z only (slide along X wall)
    else if (isTileWalkable(translateX, nextZ)) {
        translateZ = nextZ;
    }

    // only update the angle if button is being pressed
    if (deltaX != 0.0f || deltaZ != 0.0f) {
        // return angle in radians and convert to degrees
        angleRad = atan2(deltaX, deltaZ);  
        characterFacingAngle = angleRad * (180.0f / 3.14159265f);
    }

    // set camera positions
    camPos.cameraX = translateX + (sin(angle) * distance);
    camPos.cameraY = 0.6f;
    camPos.cameraZ = translateZ - (cos(angle) * distance);

    // look further down the same path the camera is facing
    camPos.targetX = translateX - (sin(angle) * lookAhead);
    camPos.targetY = 0.1f;
    camPos.targetZ = translateZ + (cos(angle) * lookAhead);

    camPos.upX = 0.0f;
    camPos.upY = 1.0f;
    camPos.upZ = 0.0f;

    return camPos;
}