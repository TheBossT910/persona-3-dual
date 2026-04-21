#include <nds.h>
#include <string.h>
#include "MusicController.h"

static FILE* s_audioFile = nullptr;
static bool s_isPaused = false;
static bool s_streamOpen = false;

static u32 s_elapsedSamples = 0;
static u32 s_loopStartSamples = 0;
static u32 s_loopEndSamples = 0;
static long s_loopStartOffset = 0;
static bool s_loopAtEOF = false;

// maxmod calls this automatically when it needs more audio
static mm_word audio_stream_callback(mm_word length, mm_addr dest, mm_stream_formats format) {
    size_t bytesReq = length * BYTES_PER_FRAME;

    if (!s_audioFile || s_isPaused) {
        memset(dest, 0, bytesReq); // output silence
        return length;
    }

    // directly read from SD to audio hardware
    size_t bytesRead = fread(dest, 1, bytesReq, s_audioFile);
    s_elapsedSamples += (bytesRead / BYTES_PER_FRAME);

    // looping logic
    bool hitLoopPoint = (s_loopEndSamples > 0 && s_elapsedSamples >= s_loopEndSamples);
    bool hitEOF = (bytesRead < bytesReq); // fread ran out of bytes

    if (hitLoopPoint || (hitEOF && s_loopAtEOF)) {
        // jump back to the loop start
        fseek(s_audioFile, s_loopStartOffset, SEEK_SET);
        s_elapsedSamples = s_loopStartSamples;
        
        // fill whatever space is left in the buffer after looping
        size_t remaining = bytesReq - bytesRead;
        if (remaining > 0) {
            size_t read2 = fread((u8*)dest + bytesRead, 1, remaining, s_audioFile);
            s_elapsedSamples += (read2 / BYTES_PER_FRAME);
            
            // if the file is incredibly short and still didn't fill, pad it
            if (read2 < remaining) {
                memset((u8*)dest + bytesRead + read2, 0, remaining - read2);
            }
        }
    } else if (hitEOF && !s_loopAtEOF) {
        // end of file reached and we are not looping. Output silence.
        memset((u8*)dest + bytesRead, 0, bytesReq - bytesRead);
    }

    return length;
}

MusicController::MusicController() {}

void MusicController::init(const char* filePath, float loopStartSeconds, float loopEndSeconds) {
    cleanup();

    s_audioFile = fopen(filePath, "rb");
    if (!s_audioFile) { iprintf("MusicController: failed to open %s\n", filePath); return; }

    s_elapsedSamples = 0;
    s_isPaused = false;

    // calculate loop points in pure samples and bytes
    s_loopStartSamples = (u32)(loopStartSeconds * AUDIO_SAMPLE_RATE);
    s_loopStartOffset = s_loopStartSamples * BYTES_PER_FRAME;

    if (loopEndSeconds == -1.0f) {
        s_loopAtEOF = true;
        s_loopEndSamples = 0;
    } else if (loopEndSeconds > 0.0f) {
        s_loopEndSamples = (u32)(loopEndSeconds * AUDIO_SAMPLE_RATE);
    } else {
        s_loopEndSamples = 0; 
    }

    // start stream
    mm_stream stream;
    stream.timer         = MM_TIMER0;
    stream.sampling_rate = AUDIO_SAMPLE_RATE;
    stream.buffer_length = 2048;
    stream.callback      = audio_stream_callback;
    stream.format        = MM_STREAM_16BIT_STEREO;
    stream.manual        = true;
    mmStreamOpen(&stream);
    s_streamOpen = true;

    mmStreamUpdate();
}

void MusicController::update() {
    if (s_streamOpen) mmStreamUpdate();
}

float MusicController::getTime() {
    return (float)s_elapsedSamples / (float)AUDIO_SAMPLE_RATE;
}

void MusicController::pause() { s_isPaused = true; }
void MusicController::resume() { s_isPaused = false; }

void MusicController::loadSFX(mm_word effectID) { mmLoadEffect(effectID); }

mm_sfxhand MusicController::playSFX(mm_word effectID, int volume, int panning) {
    mm_sound_effect effect;
    effect.id = effectID; effect.rate = (int)(1.0f * (1<<10));
    effect.handle = 0; effect.volume = volume; effect.panning = panning;
    return mmEffectEx(&effect);
}

void MusicController::stopSFX(mm_sfxhand handle) { if (handle != 0) mmEffectCancel(handle); }

void MusicController::cleanup() {
    if (s_streamOpen) {
        mmStreamClose();
        s_streamOpen = false;
    }
    if (s_audioFile) {
        fclose(s_audioFile);
        s_audioFile = nullptr;
    }
}
