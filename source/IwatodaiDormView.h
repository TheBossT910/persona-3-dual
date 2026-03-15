#pragma once
#include "View.h"

enum class TileType {
    NO_COLLISION = 0,   // walkable area
    COLLISION = 1,      // non-walkable area. Legacy = 5
    SAVE = 2,           // save interaction point
    PREV_SCENE = 3,     // load last location
    NEXT_SCENE = 4,     // load next location
    CHARACTER_Akihiko = 100
};

// implementing from View
class IwatodaiDormView : public View {
    public:
        // override tells compiler we intend to override a virtual fn in a base class (i.e. View)
        void Init() override;
        ViewState Update() override;
        void Cleanup() override;
};