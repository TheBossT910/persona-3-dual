#pragma once
#include <maxmod9.h>
#include <stdio.h>

class MusicController {
    public:
        MusicController();
        void init(const char* filePath, float loopStartSeconds, float loopEndSeconds);
        void update();
        void pause();
        float getTime();

        // loads an effect from the soundbank into memory
        void loadSFX(mm_word effectID);
        
        // plays the loaded effect and returns a handle (useful if you need to stop it early)
        // panning: 0 = left, 128 = center, 255 = right
        // volume: 0 to 255
        mm_sfxhand playSFX(mm_word effectID, int volume = 255, int panning = 128);
        
        // stops a currently playing sound effect using its handle
        void stopSFX(mm_sfxhand handle);

    private:
        void cleanup();
        int  probeFirstFrame(); // returns sample rate  
        long getAudioStartOffset(FILE* file);
        void resetStreamToOffset(long offset);
};