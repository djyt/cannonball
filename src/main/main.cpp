/***************************************************************************
    Cannonball Game Engine Code Entry Point.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

// Error reporting
#include <iostream>

// SDL Library
#include <SDL.h>
#pragma comment(lib, "SDLmain.lib") // Replace main with SDL_main
#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "glu32.lib")

// SDL Code
#include "sdl/timer.hpp"
#include "sdl/input.hpp"
#include "sdl/audio.hpp"
#include "sdl/video.hpp"

#include "romloader.hpp"
#include "stdint.hpp"
#include "engine/outrun.hpp"
#include "frontend/config.hpp"

#ifdef COMPILE_SOUND_CODE
Audio audio;
#endif

static void quit_func(int code)
{
#ifdef COMPILE_SOUND_CODE
    audio.stop_audio();
#endif
    SDL_Quit();
    exit(code);
}

static void process_events(void)
{
    SDL_Event event;

    // Grab all events from the queue.
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYDOWN:
                // Handle key presses.
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    quit_func(0);
                else
                    input.handle_key_down(&event.key.keysym);
                break;

            case SDL_KEYUP:
                input.handle_key_up(&event.key.keysym);
                break;

            case SDL_QUIT:
                // Handle quit requests (like Ctrl-c).
                quit_func(0);
                break;
        }
    }
}

// Pause Engine
bool pause_engine;

static void tick()
{
    process_events();

    if (input.has_pressed(Input::TIMER))
        config.engine.freeze_timer = !config.engine.freeze_timer;

    if (input.has_pressed(Input::PAUSE))
        pause_engine = !pause_engine;

    if (!pause_engine || input.has_pressed(Input::STEP))
    {
         // Tick Main Program Code
        outrun.tick();

        // Denote keys read
        input.frame_done();

#ifdef COMPILE_SOUND_CODE
        // Tick audio program code
        osoundint.tick();

        // Tick SDL Audio
        audio.tick();
#endif
    }
    else
    {
        // Denote keys read
        input.frame_done();
    }

    // Draw SDL Video
    video.draw_frame();  
}

static void main_loop()
{
    Timer fps;

    int t;
    int deltatime = 0;
    const int FRAME_MS = 1000 / config.fps;

    while (true)
    {
        // Start the frame timer
        fps.start();
        tick();
#ifdef COMPILE_SOUND_CODE
        deltatime = (int) (FRAME_MS * audio.adjust_speed());
#else
        deltatime = FRAME_MS;
#endif
        t = fps.get_ticks();

        // Cap Frame Rate
        if (t < deltatime)
        {
            //Sleep the remaining frame time
            SDL_Delay( deltatime - t );
        }
    }
}

int main(int argc, char* argv[])
{
    // Initialize timer and video systems
    if( SDL_Init( SDL_INIT_TIMER | SDL_INIT_VIDEO ) == -1 ) 
    { 
        std::cerr << "SDL Initialization Failed: " << SDL_GetError() << std::endl;
        return 1; 
    } 

    // Load Roms
    bool roms_loaded = roms.init();

    if (roms_loaded)
    {
        // Load XML Config
        config.load("config.xml");

        //Set the window caption 
        SDL_WM_SetCaption( "Cannonball", NULL ); 

        // Initialize SDL Video
        if (!video.init(&roms, &config.video))
            quit_func(1);

#ifdef COMPILE_SOUND_CODE
        audio.init();
#endif
        outrun.init();
        //menu.init();

        main_loop();
    }
    else
    {
        quit_func(1);
    }

    // Never Reached
    return 0;
}
