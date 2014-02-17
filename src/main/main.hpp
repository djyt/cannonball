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

    // Frame counter
	extern int frame;

    // Tick Logic. Used when running at non-standard > 30 fps
    extern bool tick_frame;

    // Millisecond Time Per Frame
    extern double frame_ms;

    // FPS Counter
    extern int fps_counter;

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