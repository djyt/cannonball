#pragma once

#include "globals.hpp"

#ifdef COMPILE_SOUND_CODE
#include "sdl/audio.hpp"
#endif

namespace cannonball
{
#ifdef COMPILE_SOUND_CODE
    extern Audio audio;
#endif

    // Millisecond Time Per Frame
    extern int frame_ms;

    // Engine Master State
    extern int state;
    
    enum
    {
        STATE_BOOT,
        STATE_INIT_MENU,
        STATE_MENU,
        STATE_INIT_GAME,
        STATE_GAME,
        STATE_QUIT
    };
}