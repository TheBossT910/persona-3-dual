#pragma once
#include <stdio.h>

class MusicController {
    public:
        MusicController();
        void init(const char* filePath, float loopStartSeconds, float loopEndSeconds);
        void update();
        void pause();

    private:
        void cleanup();
        int  probeFirstFrame(); // returns sample rate  
};