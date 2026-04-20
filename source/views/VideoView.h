#pragma once
#include <nds.h>
#include "core/View.h"
#include "core/globals.h"

// implementing from View
class VideoView : public View {    
    public:
        // override tells compiler we intend to override a virtual fn in a base class (i.e. View)
        void Init() override;
        ViewState Update() override;
        void Cleanup() override;
};