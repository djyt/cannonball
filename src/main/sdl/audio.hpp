/***************************************************************************
    SDL Audio Code.
    
    This is the SDL specific audio code.
    If porting to a non-SDL platform, you would need to replace this class.
    
    It takes the output from the PCM and YM chips, mixes them and then
    outputs appropriately.
    
    In order to achieve seamless audio, when audio is enabled the framerate
    is adjusted to essentially sync the video to the audio output.
    
    This is based upon code from the Atari800 emulator project.
    Copyright (c) 1998-2008 Atari800 development team
***************************************************************************/

#pragma once

#include "globals.hpp"

#ifdef COMPILE_SOUND_CODE

struct wav_t {
    uint8_t loaded;
    int16_t *data;
    uint32_t pos;
    uint32_t length;
};

class Audio
{
public:
    // Enable/Disable Sound
    bool sound_enabled;

    Audio();
    ~Audio();

    void init();
    void tick();
    void start_audio();
    void stop_audio();
    double adjust_speed();
    void load_wav(const char* filename);
    void clear_wav();

private:
    // Sample Rate. Can't be changed easily for now, due to lack of SDL resampling.
    static const uint32_t FREQ = 44100;

    // Stereo. Could be changed, requires some recoding.
    static const uint32_t CHANNELS = 2;

    // 16-Bit Audio Output. Could be changed, requires some recoding.
    static const uint32_t BITS = 16;

    // Low value  = Responsiveness, chance of drop out.
    // High value = Laggy, less chance of drop out.
    static const uint32_t SAMPLES  = 1024;

    // Latency (in ms) and thus target buffer size
    const static int SND_DELAY = 20;

    // allowed "spread" between too many and too few samples in the buffer (ms)
    const static int SND_SPREAD = 7;
    
    // Buffer used to mix PCM and YM channels together
    uint16_t* mix_buffer;

    wav_t wavfile;

    // Estimated gap
    int gap_est;

    // Cumulative audio difference
    double avg_gap;

    void clear_buffers();
    void pause_audio();
    void resume_audio();
};
#endif