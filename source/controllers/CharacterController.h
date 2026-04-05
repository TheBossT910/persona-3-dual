#include <stdint.h>

// TODO: move this to globals?
enum class TileType {
    NO_COLLISION = 0,   // walkable area
    COLLISION = 1,      // non-walkable area. Legacy = 5
    SAVE = 2,           // save interaction point
    PREV_SCENE = 3,     // load last location
    NEXT_SCENE = 4,     // load next location
    CHARACTER_Akihiko = 100
};

struct cameraPosition {
    float cameraX;
    float cameraY;
    float cameraZ;
    float targetX;
    float targetY;
    float targetZ;
    float upX;
    float upY;
    float upZ;
}; 

struct characterPosition {
    float x;
    float z;
    float facingAngle;
};

class CharacterController {
    public:
        // 3D environment
        const int mapWidth;
        const int mapHeight;
        const uint8_t* collisionMap;

        // world
        const float tileSize;
        const float worldOffsetX;
        const float worldOffsetZ;
        const float characterRadius;

        // movement and viewpoint
        const float speed;
        const float angleIncrement;
        const float distance; 
        const float lookAhead;

        // translation (mutable)
        float angle = 0.0;
        float translateX = 0.0;
        float translateZ = 0.0;
        float characterFacingAngle = 0.0f;

        CharacterController(
            int iMapWidth, int iMapHeight, const uint8_t* iCollisionMap, 
            float iTileSize, float iWorldOffsetX, float iWorldOffsetZ, float iCharacterRadius,
            float iSpeed, float iAngleIncrement, float iDistance, float iLookAhead,
            float iAngle, float iTranslateX, float iTranslateZ, float iCharacterFacingAngle
        ) : 
            mapWidth(iMapWidth),
            mapHeight(iMapHeight),
            collisionMap(iCollisionMap),
            tileSize(iTileSize),
            worldOffsetX(iWorldOffsetX),
            worldOffsetZ(iWorldOffsetZ),
            characterRadius(iCharacterRadius),
            speed(iSpeed),
            angleIncrement(iAngleIncrement),
            distance(iDistance),
            lookAhead(iLookAhead)
        {
            // set inital position
            angle = iAngle;
            translateX = iTranslateX;
            translateZ = iTranslateZ;
            characterFacingAngle = iCharacterFacingAngle;
        };

        cameraPosition Update(u32 keys);
        characterPosition isCharacterAt();
        TileType isTileAt();
    
    private:
        TileType isTileAt(float worldX, float worldZ);
        int isTileWalkable(float worldX, float worldZ);
};