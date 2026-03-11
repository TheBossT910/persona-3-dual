#pragma once
#include "View.h"

enum class TileType {
    NO_COLLISION = 0,
    COLLISION = 5
};

// implementing from View
class IwatodaiDormView : public View {
    public:
        // override tells compiler we intend to override a virtual fn in a base class (i.e. View)
        void Init() override;
        ViewState Update() override;
        void Cleanup() override;
};