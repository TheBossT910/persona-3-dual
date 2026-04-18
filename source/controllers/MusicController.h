#pragma once
#include <stdio.h>

class MusicController {
    public:
        MusicController();

        // Call once when your View initialises
        void init(const char* filePath, float loopStartSeconds, float loopEndSeconds);

        // Call every frame in your View's Update()
        void update();

        // Call when your View cleans up
        void cleanup();

        // Optional: pause/resume if you need it later
        void pause();
        void resume();

    private:
        int  probeFirstFrame();     // returns sample rate  
};