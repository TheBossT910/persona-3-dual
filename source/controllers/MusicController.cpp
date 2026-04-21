#include <nds.h>
#include <string.h>
#include <malloc.h>
#include "MusicController.h"

static FILE* s_audioFile = nullptr;
static bool s_isPaused = false;
static bool s_streamOpen = false;
static u32 s_elapsedSamples = 0;
static u32 s_loopStartSamples = 0;
static u32 s_loopEndSamples = 0;
static long s_loopStartOffset = 0;
static bool s_loopAtEOF = false;

// --- NEW VIDEO AUDIO GLOBALS ---
static bool s_isVideoAudio = false;
static u8* s_ringBuffer = nullptr;
static u32 s_ringBufferSize = 128 * 1024; // 128KB buffer (~1 second of audio)
static u32 s_ringReadPos = 0;
static u32 s_ringWritePos = 0;
static u32 s_ringAvailable = 0;
// -------------------------------

static mm_word audio_stream_callback(mm_word length, mm_addr dest, mm_stream_formats format) {
    size_t bytesReq = length * BYTES_PER_FRAME;

    if (s_isVideoAudio) {
        if (s_isPaused) {
            memset(dest, 0, bytesReq);
            return length;
        }

        u32 bytesToRead = bytesReq;
        if (bytesToRead > s_ringAvailable) {
            bytesToRead = s_ringAvailable;
        }

        u8* out = (u8*)dest;
        size_t firstPart = s_ringBufferSize - s_ringReadPos;
        
        if (bytesToRead <= firstPart) {
            memcpy(out, &s_ringBuffer[s_ringReadPos], bytesToRead);
            s_ringReadPos = (s_ringReadPos + bytesToRead) % s_ringBufferSize;
        } else {
            memcpy(out, &s_ringBuffer[s_ringReadPos], firstPart);
            size_t secondPart = bytesToRead - firstPart;
            memcpy(out + firstPart, s_ringBuffer, secondPart);
            s_ringReadPos = secondPart;
        }
        
        s_ringAvailable -= bytesToRead;
        s_elapsedSamples += (bytesToRead / BYTES_PER_FRAME);

        // Fill remainder with silence if the ring buffer underruns
        if (bytesToRead < bytesReq) {
            memset(out + bytesToRead, 0, bytesReq - bytesToRead);
        }
        return length;
    }

    if (!s_audioFile || s_isPaused) {
        memset(dest, 0, bytesReq); 
        return length;
    }

    size_t bytesRead = fread(dest, 1, bytesReq, s_audioFile);
    s_elapsedSamples += (bytesRead / BYTES_PER_FRAME);

    bool hitLoopPoint = (s_loopEndSamples > 0 && s_elapsedSamples >= s_loopEndSamples);
    bool hitEOF = (bytesRead < bytesReq);

    if (hitLoopPoint || (hitEOF && s_loopAtEOF)) {
        fseek(s_audioFile, s_loopStartOffset, SEEK_SET);
        s_elapsedSamples = s_loopStartSamples;
        
        size_t remaining = bytesReq - bytesRead;
        if (remaining > 0) {
            size_t read2 = fread((u8*)dest + bytesRead, 1, remaining, s_audioFile);
            s_elapsedSamples += (read2 / BYTES_PER_FRAME);
            
            if (read2 < remaining) {
                memset((u8*)dest + bytesRead + read2, 0, remaining - read2);
            }
        }
    } else if (hitEOF && !s_loopAtEOF) {
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
    s_isVideoAudio = false;

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

// --- NEW VIDEO AUDIO METHODS ---
void MusicController::initVideoAudio() {
    cleanup();

    if (!s_ringBuffer) {
        s_ringBuffer = (u8*)malloc(s_ringBufferSize);
    }
    
    s_ringReadPos = 0;
    s_ringWritePos = 0;
    s_ringAvailable = 0;
    s_elapsedSamples = 0;
    s_isPaused = false;
    s_isVideoAudio = true;

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

void MusicController::pushVideoAudio(const u8* data, size_t size) {
    if (!s_ringBuffer || !s_isVideoAudio) return;
    
    if (s_ringAvailable + size > s_ringBufferSize) {
        size = s_ringBufferSize - s_ringAvailable; // Prevent overflow
    }
    
    size_t firstPart = s_ringBufferSize - s_ringWritePos;
    if (size <= firstPart) {
        // Fits perfectly before wrapping
        memcpy(&s_ringBuffer[s_ringWritePos], data, size);
        s_ringWritePos = (s_ringWritePos + size) % s_ringBufferSize;
    } else {
        // Wraps around the end of the buffer to the beginning
        memcpy(&s_ringBuffer[s_ringWritePos], data, firstPart);
        size_t secondPart = size - firstPart;
        memcpy(s_ringBuffer, data + firstPart, secondPart);
        s_ringWritePos = secondPart;
    }
    s_ringAvailable += size;
}

float MusicController::getVideoTime() {
    return (float)s_elapsedSamples / (float)AUDIO_SAMPLE_RATE;
}
// -------------------------------

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
    if (s_isVideoAudio) {
        if (s_ringBuffer) {
            free(s_ringBuffer);
            s_ringBuffer = nullptr;
        }
        s_isVideoAudio = false;
    }
}