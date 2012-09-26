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

#include "sdl/timer.hpp" // SDL Timer for Frame Rate
#include "sdl/input.hpp" // SDL Input
#include "sdl/video.hpp" // SDL Rendering

//#include "menu.hpp"
#include "romloader.hpp"
#include "stdint.hpp"
#include "engine/outrun.hpp"

//Menu menu;

static void quit_func(int code)
{
    /*
    * Quit SDL so we can release the fullscreen
    * mode and restore the previous video settings,
    * etc.
    */
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

static void tick(void)
{
    process_events();
    outrun.tick();
  //menu.tick();
    
    return;
}

static void draw_screen(void)
{
    video.draw_frame();
    return;
}

static void main_loop(void)
{
    Timer fps;

    while(true)
    {
        //Start the frame timer
        fps.start();

        tick();
        draw_screen();

        // Cap Frame Rate
        if (fps.get_ticks() < 1000 / FRAMES_PER_SECOND)
        {
            //Sleep the remaining frame time
            SDL_Delay( ( 1000 / FRAMES_PER_SECOND ) - fps.get_ticks() );
        }
    }
}

int main(int argc, char* argv[])
{
    // Initialize all SDL subsystems
    if( SDL_Init( SDL_INIT_EVERYTHING ) == -1 ) 
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

    outrun.init();
    //menu.init();

    main_loop();

    // Never Reached
    return 0;
}
