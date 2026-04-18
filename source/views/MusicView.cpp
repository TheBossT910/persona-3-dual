// ============================================================
// MP3 playback via NitroFS file streaming + libmad + Maxmod
// ============================================================

#include <nds.h>
#include <maxmod9.h>
#include <mad.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/globals.h"
#include "MusicView.h"

// -------------------------------------------------------------------
// File streaming state
// libmad needs a contiguous buffer to decode from. We maintain a
// sliding window: when libmad reports BUFLEN (needs more data), we
// refill from the FILE*. MAD_BUFFER_GUARD zero-bytes are appended
// at EOF so libmad can flush its last frame cleanly.
// -------------------------------------------------------------------
#define MP3_FILE_PATH       "nitro:/music/song.mp3"
#define FILE_READ_CHUNK     (4 * 1024)              // How many bytes to fread at once (4 KB)
#define STREAM_BUF_SIZE     (8 * 1024)              // libmad input buffer (must be > one MP3 frame ~1.7 KB max)

static FILE*           s_mp3File    = nullptr;
static unsigned char*  s_streamBuf  = nullptr;      // Heap-allocated input buffer for libmad
static bool            s_fileEOF    = false;         // Have we hit the end of the file?

struct mad_stream madStream;
struct mad_frame  madFrame;
struct mad_synth  madSynth;

// -------------------------------------------------------------------
// Intermediate sample buffer (same as before)
// -------------------------------------------------------------------
#define MAX_SAMPLES_PER_FRAME 1152
static s16 leftoverBuffer[MAX_SAMPLES_PER_FRAME * 2];
static int leftoverCount = 0;
static int leftoverIndex = 0;

// -------------------------------------------------------------------
// Fixed-point → S16
// -------------------------------------------------------------------
inline s16 fixedToS16(mad_fixed_t sample) {
    if (sample >= MAD_F_ONE)  return 32767;
    if (sample <= -MAD_F_ONE) return -32768;
    return (s16)(sample >> (MAD_F_FRACBITS - 15));
}

// -------------------------------------------------------------------
// Refill the libmad stream buffer from the FILE*
//
// libmad tells us it needs more data via MAD_ERROR_BUFLEN.
// At that point, madStream.next_frame points to whatever partial
// data remains unconsumed (could be 0 bytes). We:
//   1. Slide that remainder to the front of our buffer
//   2. fread() a fresh chunk into the space after it
//   3. If we're at EOF, append MAD_BUFFER_GUARD zero bytes so
//      libmad can decode the very last frame without stalling
//   4. Call mad_stream_buffer() to point libmad at the refilled window
//
// Returns false if the file is exhausted AND no bytes remain.
// -------------------------------------------------------------------
static bool refillStreamBuffer() {
    // How many bytes libmad hasn't consumed yet
    size_t remaining = 0;
    if (madStream.next_frame != nullptr) {
        remaining = madStream.bufend - madStream.next_frame;
    }

    // Slide unconsumed bytes to the front
    if (remaining > 0) {
        memmove(s_streamBuf, madStream.next_frame, remaining);
    }

    size_t bytesRead = 0;
    if (!s_fileEOF) {
        // How much space is available after the leftover bytes
        size_t space = STREAM_BUF_SIZE - remaining - MAD_BUFFER_GUARD;
        size_t toRead = (space < FILE_READ_CHUNK) ? space : FILE_READ_CHUNK;

        bytesRead = fread(s_streamBuf + remaining, 1, toRead, s_mp3File);

        if (bytesRead < toRead) {
            s_fileEOF = true;
        }
    }

    size_t totalBytes = remaining + bytesRead;

    if (totalBytes == 0) {
        return false; // Truly nothing left
    }

    // Append guard bytes at EOF so libmad can flush its last frame
    if (s_fileEOF) {
        memset(s_streamBuf + totalBytes, 0, MAD_BUFFER_GUARD);
        totalBytes += MAD_BUFFER_GUARD;
    }

    mad_stream_buffer(&madStream, s_streamBuf, totalBytes);
    return true;
}

// -------------------------------------------------------------------
// Rewind the MP3 file and re-point libmad at the beginning
// (called for looping)
// -------------------------------------------------------------------
static void restartMp3() {
    fseek(s_mp3File, 0, SEEK_SET);
    s_fileEOF    = false;
    leftoverCount = 0;
    leftoverIndex = 0;

    // Finish and fully reinitialise libmad so it re-syncs from frame 0
    mad_stream_finish(&madStream);
    mad_frame_finish(&madFrame);
    mad_synth_finish(&madSynth);
    mad_stream_init(&madStream);
    mad_frame_init(&madFrame);
    mad_synth_init(&madSynth);

    // Manually prime the buffer — next_frame is null after init so
    // remaining will correctly be 0, and we just fread a fresh chunk
    size_t bytesRead = fread(s_streamBuf, 1, FILE_READ_CHUNK, s_mp3File);
    if (bytesRead < FILE_READ_CHUNK) {
        s_fileEOF = true;
        memset(s_streamBuf + bytesRead, 0, MAD_BUFFER_GUARD);
        bytesRead += MAD_BUFFER_GUARD;
    }
    mad_stream_buffer(&madStream, s_streamBuf, bytesRead);
}

// -------------------------------------------------------------------
// Init: open file + initialise libmad
// -------------------------------------------------------------------
static bool mp3Init() {
    s_mp3File = fopen(MP3_FILE_PATH, "rb");
    if (!s_mp3File) {
        iprintf("Failed to open: %s\n", MP3_FILE_PATH);
        return false;
    }

    s_streamBuf = (unsigned char*)malloc(STREAM_BUF_SIZE);
    if (!s_streamBuf) {
        iprintf("Out of memory for stream buffer!\n");
        fclose(s_mp3File);
        s_mp3File = nullptr;
        return false;
    }

    mad_stream_init(&madStream);
    mad_frame_init(&madFrame);
    mad_synth_init(&madSynth);

    s_fileEOF = false;
    refillStreamBuffer();   // Prime libmad with the first chunk
    return true;
}

// -------------------------------------------------------------------
// Maxmod stream callback — identical logic to before, but
// MAD_ERROR_BUFLEN now triggers a file refill instead of a rewind
// -------------------------------------------------------------------
mm_word mp3_stream_callback(mm_word length, mm_addr dest, mm_stream_formats format) {
    s16* output = (s16*)dest;
    mm_word samples_filled = 0;

    while (samples_filled < length) {

        // 1. Drain any leftover samples from the previous decode first
        if (leftoverCount > 0) {
            int to_copy = leftoverCount;
            if (samples_filled + to_copy > length) {
                to_copy = length - samples_filled;
            }
            for (int i = 0; i < to_copy; i++) {
                *output++ = leftoverBuffer[leftoverIndex * 2];
                *output++ = leftoverBuffer[leftoverIndex * 2 + 1];
                leftoverIndex++;
            }
            leftoverCount  -= to_copy;
            samples_filled += to_copy;
            continue;
        }

        // 2. Decode the next MP3 frame from the stream buffer
        if (mad_frame_decode(&madFrame, &madStream) != 0) {
            if (MAD_RECOVERABLE(madStream.error)) {
                continue; // Slip past a corrupted frame
            }

            if (madStream.error == MAD_ERROR_BUFLEN) {
                // libmad needs more data — refill from the file
                if (!refillStreamBuffer()) {
                    // EOF — loop back to the start
                    restartMp3();
                }
                continue;
            }

            // Fatal decode error
            break;
        }

        // 3. Synthesise PCM and stash into the leftover buffer
        mad_synth_frame(&madSynth, &madFrame);
        int  decoded_samples = madSynth.pcm.length;
        bool isStereo        = (madSynth.pcm.channels == 2);

        leftoverIndex = 0;
        leftoverCount = decoded_samples;

        for (int i = 0; i < decoded_samples; i++) {
            s16 left  = fixedToS16(madSynth.pcm.samples[0][i]);
            s16 right = isStereo ? fixedToS16(madSynth.pcm.samples[1][i]) : left;
            leftoverBuffer[i * 2]     = left;
            leftoverBuffer[i * 2 + 1] = right;
        }
        // Loop restarts, hits step 1, copies from fresh leftovers
    }

    return samples_filled;
}

// -------------------------------------------------------------------
// Cleanup
// -------------------------------------------------------------------
static void mp3Cleanup() {
    mad_synth_finish(&madSynth);
    mad_frame_finish(&madFrame);
    mad_stream_finish(&madStream);

    if (s_streamBuf) { free(s_streamBuf); s_streamBuf = nullptr; }
    if (s_mp3File)   { fclose(s_mp3File); s_mp3File   = nullptr; }
}

// -------------------------------------------------------------------
// MusicView lifecycle
// -------------------------------------------------------------------
void MusicView::Init() {
    consoleDemoInit();
    iprintf("MP3 Demo (NitroFS + Maxmod)\n");

    mm_ds_system sys;
    sys.mod_count  = 0;
    sys.samp_count = 0;
    sys.mem_bank   = 0;
    mmInit(&sys);

    if (!mp3Init()) {
        iprintf("MP3 init failed!\n");
        return;
    }

    iprintf("Decoding first frame...\n");

    // Decode one frame to probe the sample rate
    while (mad_frame_decode(&madFrame, &madStream) != 0) {
        if (madStream.error == MAD_ERROR_BUFLEN) {
            if (!refillStreamBuffer()) break;
            continue;
        }
        if (!MAD_RECOVERABLE(madStream.error)) {
            iprintf("Fatal error on first frame!\n");
            return;
        }
    }
    mad_synth_frame(&madSynth, &madFrame);

    int sampleRate = madSynth.pcm.samplerate;
    iprintf("Sample rate: %d Hz\n", sampleRate);
    iprintf("Playing...\n");

    leftoverCount = 0;
    leftoverIndex = 0;

    mm_stream stream;
    stream.timer         = MM_TIMER0;
    stream.sampling_rate = sampleRate;
    stream.buffer_length = 2304;
    stream.callback      = mp3_stream_callback;
    stream.format        = MM_STREAM_16BIT_STEREO;
    stream.manual        = true;

    mmStreamOpen(&stream);
}

ViewState MusicView::Update() {
    swiWaitForVBlank();
    scanKeys();

    if (keysDown() & KEY_START) {
        return ViewState::MAIN_MENU;
    }

    mmStreamUpdate();
    return ViewState::KEEP_CURRENT;
}

void MusicView::Cleanup() {
    setBrightness(3, 0);
    consoleClear();
    mmStreamClose();
    mp3Cleanup();
}