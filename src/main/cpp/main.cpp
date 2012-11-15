/***************************************************************************
    Cannonball Game Engine Code Entry Point.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

/*
void aMethod(aParameter p) { }  --- Pass by value. Copy is made
void aMethod(aParameter& p) { } --- Pass by reference. Copy not made

* = Access object pointed to
& = Access address pointed to

*/

// Error reporting
#include <iostream>

// SDL Library
#include <SDL.h>

// This library replaces the main function with a function called SDL_main that performs the same task.
#pragma comment(lib, "SDLmain.lib")

// This library has most SDL functions you'll use and a lot more you probably wont need.
#pragma comment(lib, "SDL.lib")

// This library defines some helper functions that make things easier for you.
#pragma comment(lib, "glu32.lib")

// SDL Code
#include "sdl/timer.hpp"
#include "sdl/input.hpp"
#include "sdl/audio.hpp"
#include "sdl/video.hpp"

//#include "menu.hpp"
#include "romloader.hpp"
#include "stdint.hpp"
#include "engine/outrun.hpp"

//Menu menu;

Audio audio;

static void quit_func(int code)
{
    audio.stop_audio();
    SDL_Quit();

    // Exit program.
    exit(code);

    return;
}

static void process_events(void)
{
    /* Our SDL event placeholder. */
    SDL_Event event;

    /* Grab all the events off the queue. */
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYDOWN:
                /* Handle key presses. */
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    quit_func(0);
                else
                    input.handle_key_down(&event.key.keysym);
                break;

            case SDL_KEYUP:
                input.handle_key_up(&event.key.keysym);
                break;

            case SDL_QUIT:
                /* Handle quit requests (like Ctrl-c). */
                quit_func(0);
                break;
        }
    }
    return;
}

static void tick()
{
    process_events();

    // Tick Main Program Code
    outrun.tick();

    // Process audio commands from main program code
    osoundint.play_queued_sound();

    // Tick audio program code
    osoundint.tick();

    // Tick SDL Audio
    audio.tick();

    // Draw SDL Video
    video.draw_frame();  
}

static void main_loop()
{
    Timer fps;

    int t;
    int deltatime = 0;
    const int FRAME_MS = 1000 / FRAMES_PER_SECOND;

    while (true)
    {
        //Start the frame timer
        fps.start();
        tick();
        deltatime = (int) (FRAME_MS * audio.adjust_speed());
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
    roms.init();

    //Set the window caption 
    SDL_WM_SetCaption( "OutRun: reassembler.blogspot.com", NULL ); 

    // Initialize SDL Video
    if (!video.init(roms.tiles.rom, roms.sprites.rom, roms.road.rom))
        quit_func(1);

    audio.init();

    outrun.init();
    //menu.init();

    main_loop();

    // Never Reached
    return 0;
}
