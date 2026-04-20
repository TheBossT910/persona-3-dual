#include <nds.h>
#include <maxmod9.h>
#include <mad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MusicController.h"

#define FILE_READ_CHUNK  (4 * 1024)
#define STREAM_BUF_SIZE  (8 * 1024)

static FILE*          s_mp3File        = nullptr;
static unsigned char* s_streamBuf      = nullptr;
static bool           s_fileEOF        = false;
static bool           s_guardAdded     = false;
static bool           s_pendingRestart = false;
static bool           s_streamOpen     = false;

static struct mad_stream madStream;
static struct mad_frame  madFrame;
static struct mad_synth  madSynth;

#define MAX_SAMPLES_PER_FRAME 1152
static s16 leftoverBuffer[MAX_SAMPLES_PER_FRAME * 2];
static int leftoverCount = 0;
static int leftoverIndex = 0;

static long  s_loopStartOffset  = 0;
static float s_loopStartSeconds = 0.0f;
static float s_loopEndSeconds   = -1.0f;    // -1.0f = play until end of file
static float s_elapsedSeconds   = 0.0f;

// internal helpers
static inline s16 fixedToS16(mad_fixed_t sample) {
    if (sample >= MAD_F_ONE)  return 32767;
    if (sample <= -MAD_F_ONE) return -32768;
    return (s16)(sample >> (MAD_F_FRACBITS - 15));
}

static void madReinit() {
    mad_stream_finish(&madStream);
    mad_frame_finish(&madFrame);
    mad_synth_finish(&madSynth);
    mad_stream_init(&madStream);
    mad_frame_init(&madFrame);
    mad_synth_init(&madSynth);
}

static bool refillStreamBuffer() {
    size_t remaining = 0;
    if (madStream.next_frame != nullptr)
        remaining = madStream.bufend - madStream.next_frame;

    if (remaining > 0)
        memmove(s_streamBuf, madStream.next_frame, remaining);

    size_t bytesRead = 0;
    if (!s_fileEOF) {
        size_t space  = STREAM_BUF_SIZE - remaining - MAD_BUFFER_GUARD;
        size_t toRead = (space < FILE_READ_CHUNK) ? space : FILE_READ_CHUNK;
        bytesRead = fread(s_streamBuf + remaining, 1, toRead, s_mp3File);
        if (bytesRead < toRead) s_fileEOF = true;
    }

    if (s_fileEOF && s_guardAdded && bytesRead == 0) return false;

    size_t totalBytes = remaining + bytesRead;
    if (totalBytes == 0) return false;

    if (s_fileEOF && !s_guardAdded) {
        memset(s_streamBuf + totalBytes, 0, MAD_BUFFER_GUARD);
        totalBytes += MAD_BUFFER_GUARD;
        s_guardAdded = true;
    }

    mad_stream_buffer(&madStream, s_streamBuf, totalBytes);
    return true;
}

static void restartMp3() {
    fseek(s_mp3File, s_loopStartOffset, SEEK_SET);
    s_fileEOF        = false;
    s_guardAdded     = false;
    leftoverCount    = 0;
    leftoverIndex    = 0;
    s_elapsedSeconds = s_loopStartSeconds;  // reset to loop start time, not 0
    madReinit();

    size_t bytesRead = fread(s_streamBuf, 1, FILE_READ_CHUNK, s_mp3File);
    if (bytesRead < FILE_READ_CHUNK) {
        s_fileEOF = true;
        memset(s_streamBuf + bytesRead, 0, MAD_BUFFER_GUARD);
        bytesRead += MAD_BUFFER_GUARD;
    }
    mad_stream_buffer(&madStream, s_streamBuf, bytesRead);
}

// decode and discard frames until we reach the target time,
// then snapshot the file position as the loop point
static long findOffsetAtTime(float targetSeconds) {
    float elapsed = 0.0f;

    while (elapsed < targetSeconds) {
        while (mad_frame_decode(&madFrame, &madStream) != 0) {
            if (madStream.error == MAD_ERROR_BUFLEN) {
                if (!refillStreamBuffer()) return -1;
                continue;
            }
            if (!MAD_RECOVERABLE(madStream.error)) return -1;
        }

        mad_synth_frame(&madSynth, &madFrame);

        // Each frame's duration in seconds
        float frameDuration = (float)madSynth.pcm.length / (float)madSynth.pcm.samplerate;
        elapsed += frameDuration;
    }

    // snapshot where we are in the file right now
    // next_frame points to the start of the next unread frame in the buffer
    // we calculate its actual file position from how much buffer is ahead of it
    size_t bufferAhead = madStream.bufend - madStream.next_frame;
    long filePos = ftell(s_mp3File) - (long)bufferAhead;
    return filePos;
}

// maxmod stream callback
static mm_word mp3_stream_callback(mm_word length, mm_addr dest, mm_stream_formats format) {
    s16* output = (s16*)dest;
    mm_word samples_filled = 0;

    if (s_pendingRestart) {
        s_pendingRestart = false;
        restartMp3();
    }

    while (samples_filled < length) {
        if (leftoverCount > 0) {
            int to_copy = leftoverCount;
            if ((mm_word)(samples_filled + to_copy) > length)
                to_copy = length - samples_filled;

            for (int i = 0; i < to_copy; i++) {
                *output++ = leftoverBuffer[leftoverIndex * 2];
                *output++ = leftoverBuffer[leftoverIndex * 2 + 1];
                leftoverIndex++;
            }
            leftoverCount  -= to_copy;
            samples_filled += to_copy;
            continue;
        }

        // Check if we've hit the loop end point
        if (s_loopEndSeconds > 0.0f && s_elapsedSeconds >= s_loopEndSeconds) {
            s_pendingRestart = true;
            break;
        }

        if (mad_frame_decode(&madFrame, &madStream) != 0) {
            if (MAD_RECOVERABLE(madStream.error)) continue;
            if (madStream.error == MAD_ERROR_BUFLEN) {
                if (!refillStreamBuffer()) {
                    s_pendingRestart = true;
                    break;
                }
                continue;
            }
            break;
        }

        mad_synth_frame(&madSynth, &madFrame);
        int  decoded_samples = madSynth.pcm.length;
        bool isStereo        = (madSynth.pcm.channels == 2);

        // Track elapsed time — one frame's worth per decode
        s_elapsedSeconds += (float)decoded_samples / (float)madSynth.pcm.samplerate;

        leftoverIndex = 0;
        leftoverCount = decoded_samples;

        for (int i = 0; i < decoded_samples; i++) {
            s16 left  = fixedToS16(madSynth.pcm.samples[0][i]);
            s16 right = isStereo ? fixedToS16(madSynth.pcm.samples[1][i]) : left;
            leftoverBuffer[i * 2]     = left;
            leftoverBuffer[i * 2 + 1] = right;
        }
    }

    while (samples_filled < length) {
        *output++ = 0;
        *output++ = 0;
        samples_filled++;
    }

    return length;
}

MusicController::MusicController() {}

int MusicController::probeFirstFrame() {
    int sampleRate = 0;
    int confirmedRate = 0;
    int confirmCount = 0;

    // decode until we see the same sample rate 3 times in a row
    // this skips VBR/Xing header frames which often report wrong rates
    while (confirmCount < 3) {
        while (mad_frame_decode(&madFrame, &madStream) != 0) {
            if (madStream.error == MAD_ERROR_BUFLEN) {
                if (!refillStreamBuffer()) break;
                continue;
            }
            if (!MAD_RECOVERABLE(madStream.error)) break;
        }
        mad_synth_frame(&madSynth, &madFrame);
        int detectedRate = madSynth.pcm.samplerate;

        if (detectedRate == confirmedRate) {
            confirmCount++;
        } else {
            confirmedRate = detectedRate;
            confirmCount = 1;
        }
    }
    sampleRate = confirmedRate;
    // iprintf("Confirmed sample rate: %d Hz\n", sampleRate);
    return sampleRate;
}

// ID3 parsing logic
long MusicController::getAudioStartOffset(FILE* file) {
    if (!file) return 0;
    
    unsigned char id3Header[10];
    if (fread(id3Header, 1, 10, file) == 10) {
        if (id3Header[0] == 'I' && id3Header[1] == 'D' && id3Header[2] == '3') {
            // ID3v2 size is stored as a 28-bit sync-safe integer
            int id3Size = (id3Header[6] << 21) | (id3Header[7] << 14) | (id3Header[8] << 7) | id3Header[9];
            return id3Size + 10;
        }
    }
    return 0; // no ID3v2 tag found, start at 0
}

void MusicController::resetStreamToOffset(long offset) {
    fseek(s_mp3File, offset, SEEK_SET);
    s_fileEOF    = false;
    s_guardAdded = false;
    
    madReinit();
    refillStreamBuffer();
    
    // reset leftover tracking (discard any previous frame's audio)
    leftoverCount = 0;
    leftoverIndex = 0;
}

void MusicController::init(const char* filePath, float loopStartSeconds = 0.0f, float loopEndSeconds = -1.0f) {
    // cleanup any previous audio streams
    cleanup();

    s_loopEndSeconds = loopEndSeconds;
    s_elapsedSeconds = 0.0f;
    s_pendingRestart = false;

    s_mp3File = fopen(filePath, "rb");
    if (!s_mp3File) { iprintf("MusicController: failed to open %s\n", filePath); return; }

    s_streamBuf = (unsigned char*)malloc(STREAM_BUF_SIZE);
    if (!s_streamBuf) { iprintf("MusicController: out of memory\n"); fclose(s_mp3File); return; }

    // find where the real audio starts
    long audioStartOffset = getAudioStartOffset(s_mp3File);

    // prep the stream to read the first frame
    resetStreamToOffset(audioStartOffset);

    // probe to get the sample rate
    int sampleRate = probeFirstFrame();
    if (sampleRate < 0) { iprintf("MusicController: decode error\n"); return; }

    // find and store the loop point
    if (loopStartSeconds > 0.0f) {
        s_loopStartSeconds = loopStartSeconds;
        s_loopStartOffset  = findOffsetAtTime(loopStartSeconds);
    } else {
        s_loopStartSeconds = 0.0f;
        s_loopStartOffset  = audioStartOffset;
    }

    // reset stream back to the beginning for actual playback
    resetStreamToOffset(audioStartOffset);

    // start Maxmod
    mm_stream stream;
    stream.timer         = MM_TIMER0;
    stream.sampling_rate = sampleRate;
    stream.buffer_length = 2304;
    stream.callback      = mp3_stream_callback;
    stream.format        = MM_STREAM_16BIT_STEREO;
    stream.manual        = true;
    mmStreamOpen(&stream);
    s_streamOpen = true;

    // fill stream
    mmStreamUpdate();
}

void MusicController::update() {
    mmStreamUpdate();
}

void MusicController::cleanup() {
    if (s_streamOpen) {
        mmStreamClose();
        s_streamOpen = false;
    }

    mad_synth_finish(&madSynth);
    mad_frame_finish(&madFrame);
    mad_stream_finish(&madStream);

    if (s_streamBuf) { free(s_streamBuf);  s_streamBuf = nullptr; }
    if (s_mp3File) { fclose(s_mp3File);  s_mp3File   = nullptr; }
}

void MusicController::pause()  {
    if (s_streamOpen) {
        mmStreamClose();
        s_streamOpen = false;
    }
}