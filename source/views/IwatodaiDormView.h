#pragma once
#include "core/View.h"
#include "controllers/DialogueController.h"
#include "controllers/CharacterController.h"
#include "controllers/MusicController.h"

// implementing from View
class IwatodaiDormView : public View {
    public:
        // override tells compiler we intend to override a virtual fn in a base class (i.e. View)
        void Init() override;
        ViewState Update() override;
        void Cleanup() override;

    private:
        // sub screen
        int bgAkihiko;
        PrintConsole console;

        // controllers
        CharacterController* playerCtrl;
            // camera pos
            cameraPosition camPos;
            // world
            const float tileSize = 0.0625f;
            const float worldOffsetX = 1.8125f;
            const float worldOffsetZ = 1.6875f;
            const float characterRadius = 0.05f;
            // movement and viewpoint
            const float speed = 0.01f;
            const float angleIncrement = 0.05f;
            const float distance = 0.5f; 
            const float lookAhead = 0.3f;
            // set character initial translation position
            const float translateX = -1.3;
            const float translateZ = -0.8;
            const float angle = -1.6;
            const float characterFacingAngle = 91.67;
        DialogueController dialogueCtrl;
            dialogue lines[5];
};