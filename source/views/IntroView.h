#pragma once
#include "core/View.h"
#include "controllers/MusicController.h"

//a simple sprite structure
typedef struct
{
   u16* gfx;
   SpriteSize size;
   SpriteColorFormat format;
   int rotationIndex;
   int paletteAlpha;
   int x;
   int y;
} Sprite;

// implementing from View
class IntroView : public View {
    private:
        // controllers
        MusicController musicCtrl;

        Sprite logoSprite[2];
        int bg[4];
        touchPosition touchXY;
        
        // for silhouette animation
        int silhouetteX = -256;
        int silhouetteY = 192;
        
        // for bottom screen text animation
        bool animateText = false;
        int duration = 4;
        int durationCounter = 0;
        int textAlpha = 0;
        int textAlphaDirection = 0;
        
        // for logoSprite
        bool displayLogo = false;
        int logoOpacity = 0;
        
        // for overlayBackground
        bool displayOverlay = false;
        int overlayOpacity = 0;
        // NOTE: we use u16 to allow overflow (and naturally reset values back to 0)
        u16 waveAngle = 0;
        u16 currentRotation = 0;
        int baseSpeed = 20;
        int fluctuation = 50;
    
    public:
        // override tells compiler we intend to override a virtual fn in a base class (i.e. View)
        void Init() override;
        ViewState Update() override;
        void Cleanup() override;
};