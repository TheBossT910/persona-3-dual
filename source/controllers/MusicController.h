#pragma once
#include <stdio.h>

class MusicController {
    public:
        MusicController();
        void init(const char* filePath, float loopStartSeconds, float loopEndSeconds);
        void update();
        void pause();
        float getTime();

    private:
        void cleanup();
        int  probeFirstFrame(); // returns sample rate  
        long getAudioStartOffset(FILE* file);
        void resetStreamToOffset(long offset);
};