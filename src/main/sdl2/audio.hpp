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
#include "frontend/config.hpp"
#include <SDL.h>

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
    void pause_audio();
    void resume_audio();

private:
    // Sample Rate. Can't be changed easily for now, due to lack of SDL resampling.
    static const uint32_t FREQ = 31250;

    // Stereo. Could be changed, requires some recoding.
    static const uint32_t CHANNELS = 2;

    // 16-Bit Audio Output. Could be changed, requires some recoding.
    static const uint32_t BITS = 16;

    // four generated audio frames per callback, this implies 32ms delay in SDL
    static const uint32_t SAMPLES  = 1000;

    // how many fragments (of SAMPLES) in the dsp buffer
    static const int DSP_BUFFER_FRAGS = 6;

    // Added delay between sound generation and playback, in ms. Reduces underruns.
    // Must be less than the overall buffer size allows (DSP_BUFFER_FRAGS x 8ms)
    // The overall audio delay will be the SDL delay plus this offset.
    // More delay means less chance of drop-out (i.e. buffer underrun somewhere)
    // Typically, audio delayed more than 100ms will be noticably lagging.
    // However, the video also have a delay of approx. 16 or 30ms, depending on the
    // frame rate.
    const static int SND_DELAY = 40;

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

    // SDL2 audio device
    SDL_AudioDeviceID dev;
};
#endif
