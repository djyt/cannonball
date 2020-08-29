/***************************************************************************
    Cannonball Main Entry Point.

    Copyright (c) 2013, 2020, Chris White.

    Multi-threading added by James Pearce, enables game to run smoothly
    at whatever frame rate the machine can achieve, and corrects audio
    playback speed to match the arcade.

    See license.txt for more details.
***************************************************************************/

// Error reporting
#include <iostream>

// SDL Library
#include <SDL.h>
#ifdef _WIN32
#ifndef SDL2
#pragma comment(lib, "SDLmain.lib") // Replace main with SDL_main
#endif
#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "glu32.lib")
#endif

// SDL Specific Code
#if defined SDL2
#include "sdl2/timer.hpp"
#include "sdl2/input.hpp"
#else
#include "sdl/timer.hpp"
#include "sdl/input.hpp"
#endif

#include "video.hpp"

#include "romloader.hpp"
#include "trackloader.hpp"
#include "stdint.hpp"
#include "main.hpp"
#include "setup.hpp"
#include "engine/outrun.hpp"
#include "frontend/config.hpp"
#include "frontend/menu.hpp"

#include "cannonboard/interface.hpp"
#include "engine/oinputs.hpp"
#include "engine/ooutputs.hpp"
#include "engine/omusic.hpp"

// Direct X Haptic Support.
// Fine to include on non-windows builds as dummy functions used.
#include "directx/ffeedback.hpp"

//Multi-threading to enable seperate audio and game threads, as on the S16 system board
#include <thread>
#include <mutex>
#include <time.h>
#include <omp.h>

// Initialize Shared Variables
using namespace cannonball;

int    cannonball::state       = STATE_BOOT;
double cannonball::frame_ms    = 0;
int    cannonball::frame       = 0;
bool   cannonball::tick_frame  = true;
int    cannonball::fps_counter = 0;

std::mutex mainMutex; // used to sequence audio/game/sdl threads

#ifdef COMPILE_SOUND_CODE
Audio cannonball::audio;
#endif

Menu* menu;
Interface cannonboard;


// Processing stats, displayed on console when game is terminated.
uint32_t frames = 0;
uint32_t lateframes = 0;
uint32_t runtime = 0;
uint32_t enginetime = 0;
uint32_t s16videotime = 0;
uint32_t rendertime = 0;
uint32_t maxlocktime = 0;

static void quit_func(int code)
{
    input.close();
    forcefeedback::close();
    delete menu;
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
                    state = STATE_QUIT;
                else
                    input.handle_key_down(&event.key.keysym);
                break;

            case SDL_KEYUP:
                input.handle_key_up(&event.key.keysym);
                break;

            case SDL_JOYAXISMOTION:
                input.handle_joy_axis(&event.jaxis);
                break;

            case SDL_JOYBUTTONDOWN:
                input.handle_joy_down(&event.jbutton);
                break;

            case SDL_JOYBUTTONUP:
                input.handle_joy_up(&event.jbutton);
                break;

            case SDL_QUIT:
                // Handle quit requests (like Ctrl-c).
                state = STATE_QUIT;
                break;
        }
    }
}

// Pause Engine
bool pause_engine;

static void tick()
{
    frame++;

    // Get CannonBoard Packet Data
    Packet* packet = config.cannonboard.enabled ? cannonboard.get_packet() : NULL;

    // Non standard FPS.
    // Determine whether to tick the current frame.
    if (config.fps != 30)
    {
        if (config.fps == 60)
            tick_frame = frame & 1;
        else if (config.fps == 120)
            tick_frame = (frame & 3) == 1;
    }

    process_events();

    if (tick_frame)
        oinputs.tick(packet); // Do Controls
    oinputs.do_gear();        // Digital Gear

    switch (state)
    {
        case STATE_GAME:
        {
            if (input.has_pressed(Input::TIMER))
                outrun.freeze_timer = !outrun.freeze_timer;

            if (input.has_pressed(Input::PAUSE))
                pause_engine = !pause_engine;

            if (input.has_pressed(Input::MENU))
                state = STATE_INIT_MENU;

            if (!pause_engine || input.has_pressed(Input::STEP))
            {
                outrun.tick(packet, tick_frame);
                input.frame_done(); // Denote keys read
            }
            else
            {
                input.frame_done(); // Denote keys read
            }
        }
        break;

        case STATE_INIT_GAME:
            if (config.engine.jap && !roms.load_japanese_roms())
            {
                state = STATE_QUIT;
            }
            else
            {
                pause_engine = false;
                outrun.init();
                state = STATE_GAME;
            }
            break;

        case STATE_MENU:
        {
            menu->tick(packet);
            input.frame_done();
        }
        break;

        case STATE_INIT_MENU:
            oinputs.init();
            outrun.outputs->init();
            menu->init();
            state = STATE_MENU;
            break;
    }
    // Write CannonBoard Outputs
    if (config.cannonboard.enabled)
        cannonboard.write(outrun.outputs->dig_out, outrun.outputs->hw_motor_control);

}


static void main_sound_loop()
{
    // This thread basically does what the Z80 on the S16 board is responsible for: the sound.
    // osound.tick() is called 125 times per second with isolated access to the game engine
    // via the mutex lock. audio.tick() the outputs the samples via SDL outside of the lock.
    double targetupdatetime = 0; // our start point
    double interval = 1000.0 / 125.0; // this is the fixed playback rate.
    double waittime; // time to wait in ms
    struct timespec ts;
    int res; int audiotick = 0;
    long thislocktime = 0;

    ts.tv_sec = 0; // whole seconds to sleep

    audio.init(); // initialise the audio, but hold in paused state as nothing to play yet
    while ((state != STATE_MENU) && (state != STATE_GAME))
        SDL_Delay(long(interval)); // await game engine initialisation

    // here we go!
    targetupdatetime = double(SDL_GetTicks()); // our start point
    while (state != STATE_QUIT) {
        // check if another thread is running and wait if it is
        mainMutex.lock();
        targetupdatetime += interval;
        thislocktime = SDL_GetTicks();
        osoundint.tick(); // generate the game sounds, music etc
        mainMutex.unlock();
        thislocktime = SDL_GetTicks() - thislocktime;
        if (thislocktime > maxlocktime) maxlocktime = thislocktime;

        // now pass things over to SDL, on first pass this will enable the audio too
        audio.tick();

        // and sleep until the end of the cycle
        waittime = targetupdatetime - double(SDL_GetTicks());
        if (waittime > (interval+1))
            // reset timer, likely SDL_GetTicks wrapped
            targetupdatetime = double(SDL_GetTicks());
        else if (waittime > 0) {
            // wait a maximum of one update interval
            ts.tv_nsec = long(waittime * 1000000); // ns to sleep
            res = nanosleep(&ts, &ts);
        } else if (waittime < -64) {
            // we are more than 64ms behind; perhaps the video mode was reset
            audio.resume_audio(); // clears buffers and re-establishes delay
            targetupdatetime = double(SDL_GetTicks()); // reset timer
        }
    }
    audio.stop_audio(); // we're done
}

static void main_video_loop()
{
    // This thread decouples the game logic from the display via SDL, which could
    // be slow because of the CRT filtering, upscaling, and just lack of performance.
    // Essentially, this thread behaves lives the arcade graphics card, rendering
    // all aspects of the image whilst the game and audio threads march on.
    double targetupdatetime = 0; // our start point
    double interval;
    double waittime; // time to wait in ms
    double s16videostart, renderstart;
    struct timespec ts;
    int    res;
    long   thislocktime = 0;

    ts.tv_sec = 0; // whole seconds to sleep
    while (state != STATE_QUIT)
    {
        frames++;
        interval = 1000.0 / double( config.fps ); // screen refresh interval

        // check if another thread is running and wait if it is
        mainMutex.lock();
        if (!targetupdatetime) targetupdatetime = double(SDL_GetTicks()); // our start point
        targetupdatetime += interval; // rolling target
        thislocktime = SDL_GetTicks();
        // acquire the video frame when available
        // draw_frame() emulates the S16 hardware graphics, producing the pixel array from game data
        s16videostart = SDL_GetTicks();
        video.draw_frame();
        mainMutex.unlock();
        thislocktime = SDL_GetTicks() - thislocktime;
        if (thislocktime > maxlocktime) maxlocktime = thislocktime;

        // and get to work on it with SDL, Blargg etc. This can run along side other stuff as
        // it doesn't touch the game engine.
        renderstart = SDL_GetTicks();
        video.finalize_frame();
        rendertime += (SDL_GetTicks() - renderstart);
        s16videotime += (renderstart - s16videostart);

        // now sleep until the end of the cycle
        waittime = targetupdatetime - double(SDL_GetTicks());
        if (waittime > (interval+1))
            // reset timer, likely SDL_GetTicks wrapped
            targetupdatetime = double(SDL_GetTicks());
        else if (waittime > 0) {
            // wait a maximum of one update interval
            ts.tv_nsec = long(waittime * 1000000); // ns to sleep
            res = nanosleep(&ts, &ts);
        }
    }
}


static void main_game_loop()
{
    // This thread controls the game.
    // FPS Counter (If Enabled)
    Timer fps_count;
    int frame = 0;
    fps_count.start();

    // General Frame Timing
    int t;
    double waittime;
    double interval;
    double targetupdatetime = 0;
    struct timespec ts;
    int res;
    double enginestart;
    long thislocktime = 0;

    ts.tv_sec = 0; // whole seconds to sleep

    SDL_Delay(250); // wait for other threads to spawn

    // and here we go!
    runtime = SDL_GetTicks(); // get the time the game was started; we'll take that off at the end

    while (state != STATE_QUIT)
    {
        interval = 1000.0 / double( config.fps ); // screen refresh interval

        // check if another thread is running and wait if it is
        mainMutex.lock();
        if (!targetupdatetime) targetupdatetime = double(SDL_GetTicks()); // our start point
        targetupdatetime += interval; // rolling target
        thislocktime = SDL_GetTicks();
        enginestart = SDL_GetTicks();
        tick(); // control what's happening
        mainMutex.unlock();
        enginetime += (SDL_GetTicks() - enginestart);
        thislocktime = SDL_GetTicks() - thislocktime;
        if (thislocktime > maxlocktime) maxlocktime = thislocktime;

        // update fps counter, if running
        if (config.video.fps_count) {
            frame++;
            if (fps_count.get_ticks() >= 1000) {
                // One second has elapsed
                fps_counter = frame;
                frame       = 0;
                fps_count.start();
            }
        }

        // now sleep until the end of the cycle
        waittime = targetupdatetime - double(SDL_GetTicks());
        if (waittime > (interval+1))
            // reset timer, likely SDL_GetTicks wrapped
            targetupdatetime = double(SDL_GetTicks());
        else if (waittime > 0) {
            // wait a maximum of one update interval
            ts.tv_nsec = long(waittime * 1000000); // ns to sleep
            res = nanosleep(&ts, &ts);
        } else lateframes++;
    }

    // JJP - get overall application run time at end of game and display the runtime stats.
    runtime = SDL_GetTicks() - runtime; // total ms the game ran
    SDL_Delay(100); // wait for other threads to finish
    printf("Stats:\n");
    printf("\n");
    printf("Run Time                :  %5.1fs           \n",(float(runtime)/1000.0));
    printf("Target fps              :  %3i.0            \n",config.fps);
    printf("Actual fps              :  %5.1f            \n",((float(frames))/(float(runtime)/1000)));
    printf("Late frames             :  %3i.0  (%4.1f%%) \n",lateframes,(float(lateframes*100)/float(frames)));
    printf("Game engine             :  %5.1fs (%4.1f%%) - %2.1fms/frame\n",
        (float(enginetime)/1000.0),(float(enginetime*100)/float(runtime)),(float(enginetime)/float(frames)));
    printf("S16 Video Emulation     :  %5.1fs (%4.1f%%) - %2.1fms/frame\n",
        (float(s16videotime)/1000.0),(float(s16videotime*100)/float(runtime)),(float(s16videotime)/float(frames)));
    printf("Game rendering          :  %5.1fs (%4.1f%%) - %2.1fms/frame\n",
        (float(rendertime)/1000.0),(float(rendertime*100)/float(runtime)),(float(rendertime)/float(frames)));
    printf("Maximum thread lock     :   %i  ms\n",maxlocktime);
    printf("\n");
}


int main(int argc, char* argv[])
{
    printf("OMP Max Threads: %i\n",omp_get_max_threads());

    // Initialize timer and video systems
    if( SDL_Init( SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) == -1 ) {
        std::cerr << "SDL Initialization Failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    menu = new Menu(&cannonboard);

    bool loaded = false;

    // Load LayOut File
    if (argc == 3 && strcmp(argv[1], "-file") == 0) {
        if (trackloader.set_layout_track(argv[2]))
            loaded = roms.load_revb_roms();
    }
    // Load Roms Only
    else loaded = roms.load_revb_roms();

    if (loaded)
    {
        // Load XML Config
        config.load(FILENAME_CONFIG);

        // Load fixed PCM ROM based on config
        if (config.sound.fix_samples)
            roms.load_pcm_rom(true);

        // Load patched widescreen tilemaps
        if (!omusic.load_widescreen_map())
            std::cout << "Unable to load widescreen tilemaps" << std::endl;

#ifndef SDL2
        //Set the window caption 
        SDL_WM_SetCaption( "Cannonball", NULL ); 
#endif

        // Initialize SDL Video
        if (!video.init(&roms, &config.video))
            quit_func(1);

        state = config.menu.enabled ? STATE_INIT_MENU : STATE_INIT_GAME;

        // Initalize controls
        input.init(config.controls.pad_id,
                   config.controls.keyconfig, config.controls.padconfig,
                   config.controls.analog,    config.controls.axis, config.controls.asettings);

        if (config.controls.haptic)
            config.controls.haptic = forcefeedback::init(config.controls.max_force, config.controls.min_force, config.controls.force_duration);

        // Initalize CannonBoard (For use in original cabinets)
        if (config.cannonboard.enabled)
        {
            cannonboard.init(config.cannonboard.port, config.cannonboard.baud);
            cannonboard.start();
        }

        // Populate menus
        menu->populate();

        // start the game threads
        std::thread video (main_video_loop); // SDL rendering thread
        std::thread sound (main_sound_loop); // Z80 thread
        std::thread game (main_game_loop);   // Primary M68k thread
//        std::thread road (main_road_loop);   // Secondary M68k thread
//        road.join();
        game.join();
        sound.join();
        video.join(); // wait for all threads to complete
        quit_func(0);
    }
    else quit_func(1);
    // Never Reached
    return 0;
}
