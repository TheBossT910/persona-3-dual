#pragma once
#include "core/View.h"

// implementing from View
class MusicView : public View { 
    public:
        int sampleRate;
        // override tells compiler we intend to override a virtual fn in a base class (i.e. View)
        void Init() override;
        ViewState Update() override;
        void Cleanup() override;
        
};