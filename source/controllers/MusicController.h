#pragma once
#include <maxmod9.h>
#include <stdio.h>

#define AUDIO_SAMPLE_RATE 32000
#define AUDIO_CHANNELS 2
#define BYTES_PER_SAMPLE 2
#define BYTES_PER_FRAME (AUDIO_CHANNELS * BYTES_PER_SAMPLE)

class MusicController {
    public:
        MusicController();
        void init(const char* filePath, float loopStartSeconds = 0.0f, float loopEndSeconds = -1.0f);
        void update();
        void pause();
        void resume();
        float getTime();

        // --- NEW VIDEO AUDIO METHODS ---
        void initVideoAudio();
        void pushVideoAudio(const u8* data, size_t size);
        float getVideoTime();
        // -------------------------------

        void loadSFX(mm_word effectID);
        mm_sfxhand playSFX(mm_word effectID, int volume = 255, int panning = 128);
        void stopSFX(mm_sfxhand handle);
        void cleanup();

    private:
        long getAudioStartOffset(FILE* file);
};