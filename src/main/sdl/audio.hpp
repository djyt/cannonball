/***************************************************************************
    
    Emscripten Audio Handler.  
    
***************************************************************************/

#pragma once

#include "../globals.hpp"

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
    void tick(float*, float*);
    void start_audio();
    void stop_audio();
    double adjust_speed();
    void load_wav(const char* filename);
    void clear_wav();

private:
    wav_t wavfile;
    void pause_audio();
    void resume_audio();
};
#endif