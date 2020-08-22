/***************************************************************************
    Cannonball Main Entry Point.

    Copyright Chris White.
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

// Initialize Shared Variables
using namespace cannonball;

int    cannonball::state       = STATE_BOOT;
double cannonball::frame_ms    = 0;
int    cannonball::frame       = 0;
bool   cannonball::tick_frame  = true;
int    cannonball::fps_counter = 0;

std::mutex mainMutex;

#ifdef COMPILE_SOUND_CODE
Audio cannonball::audio;
#endif

Menu* menu;
Interface cannonboard;


// JJP processing stats
uint32_t frames = 0;
uint32_t lateframes = 0;
uint32_t runtime = 0;
uint32_t enginetime = 0;
uint32_t renderingtime = 0;
uint32_t idletime = 0;
long soundframe = 0;


static void quit_func(int code)
{
#ifdef COMPILE_SOUND_CODE
    audio.stop_audio();
#endif
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

                #ifdef COMPILE_SOUND_CODE
                // Tick audio program code
//                osoundint.tick();
                // Tick SDL Audio
                audio.tick();
                #endif
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
            #ifdef COMPILE_SOUND_CODE
            // Tick audio program code
//            osoundint.tick();
            // Tick SDL Audio
            audio.tick();
            #endif
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
    // osound.tick() is called at an interval of music target BPM / 60
    double targetupdatetime = double(SDL_GetTicks()); // our start point
    int waittime; // time to wait in ms
    struct timespec ts;
    int res;

    ts.tv_sec = 0; // whole seconds to sleep
    while (state != STATE_QUIT)
    {
        if ((config.sound.playback_speed>=120) &&
            (config.sound.playback_speed<=136))
            targetupdatetime += 1000.0 / double( config.sound.playback_speed );
        else targetupdatetime += 1000.0 / 125.0;
        soundframe++; // debug
        // check if the game another thread is running and wait if it is
        ts.tv_nsec = 1000; // 1us sleep period, if we need to wait
        mainMutex.lock();
        osoundint.tick(); // process Outrun engine audio
        mainMutex.unlock();
        waittime = int( targetupdatetime - double(SDL_GetTicks()) );
        if ( (waittime > 0) && (waittime <= ((1000 / config.sound.playback_speed)+1)) ) {
            // wait a maximum of one update interval
            ts.tv_nsec = waittime * 1000000; // ns to sleep
            res = nanosleep(&ts, &ts);
        }
    }
}

static void main_game_loop()
{
    // This thread does the job of everything except the sound
    // FPS Counter (If Enabled)
    Timer fps_count;
    int frame = 0;
    fps_count.start();

    // General Frame Timing
//    Timer frame_time;
    int t; int waittime; int drop_next_frame = 0;
    double targetframetime = 0;
    double deltatime  = 0;
    int deltaintegral = 0;
    struct timespec ts;
    int res;
long lastsoundframe = -1; // debug
    // JJP - performance counters
    uint32_t enginestart;
    uint32_t renderingstart;
//    sleep(1); // wait for system to stabalise
    runtime = SDL_GetTicks(); // get the time the game was started; we'll take that off at the end
    targetframetime = runtime;
    ts.tv_sec = 0; // whole seconds to sleep
//    printf("frame_ms %f; fix_bugs: %i\n",frame_ms,config.engine.fix_bugs);

    while (state != STATE_QUIT)
    {
        targetframetime += frame_ms;
        if (soundframe == lastsoundframe) state = STATE_QUIT; // sound thread crashed
        lastsoundframe = soundframe;
//        frame_time.start();
        frames++;
        mainMutex.lock();
        // JJP - various state timers to assist with thread optimisation on different platforms
        enginestart = SDL_GetTicks();
        tick(); // control what's happening
        mainMutex.unlock();
        enginetime += (SDL_GetTicks() - enginestart);
        // Draw SDL Video
        renderingstart = SDL_GetTicks();
        if (drop_next_frame) drop_next_frame = 0;
        else video.draw_frame();
        renderingtime += (SDL_GetTicks() - renderingstart);

        waittime = int( targetframetime - double(SDL_GetTicks()) );

        if (waittime > int(frame_ms+1))
            // likely SDL_GetTicks() has wrapped; reset timer)
               targetframetime = double(SDL_GetTicks());

        if (-waittime > int(frame_ms+1)) {
           // Frame is late
           lateframes++;
           if (-waittime > (2*int(frame_ms))) {
               // we don't even have time to attempt drawing the next frame;
               // just reset the reference point.
               printf("INFO: Game Loop: Resetting game loop timer.\n");
               targetframetime = double(SDL_GetTicks());
           } else {
               printf("INFO: Game Loop: Frame dropped.\n");
               drop_next_frame = 1; // don't draw next frame, to catch up
           }
       } else {
            if (waittime > 0) {
                // wait for specified period, which is a maximum of one frame
                ts.tv_nsec = waittime * 1000000; // ns to sleep
                res = nanosleep(&ts, &ts);
            }
        }

        if (config.video.fps_count)
        {
            frame++;
            // One second has elapsed
            if (fps_count.get_ticks() >= 1000)
            {
                fps_counter = frame;
                frame       = 0;
                fps_count.start();
            }
        }
    }

    // JJP - get overall application run time at end of game and display the runtime stats.
    runtime = SDL_GetTicks() - runtime; // total ms the game ran
    printf("Stats:\n");
    printf("\n");
    printf("Run Time                :  %5.1fs           \n",(float(runtime)/1000.0));
    printf("Frames                  : %4i.0            \n",frames);
    printf("Target fps              :  %3i.0            \n",config.fps);
    printf("Actual fps              :  %5.1f            \n",((float(frames))/(float(runtime)/1000)));
    printf("Late frames             :  %3i.0  (%4.1f%%) \n",lateframes,(float(lateframes*100)/float(frames)));
    printf("Game engine             :  %5.1fs (%4.1f%%) - %2.1fms/frame\n",
        (float(enginetime)/1000.0),(float(enginetime*100)/float(runtime)),(float(enginetime)/float(frames)));
    printf("Video rendering         :  %5.1fs (%4.1f%%) - %2.1fms/frame\n",
        (float(renderingtime)/1000.0),(float(renderingtime*100)/float(runtime)),(float(renderingtime)/float(frames)));
    printf("Idle                    :  %5.1fs (%4.1f%%) \n",(float(idletime)/1000.0),(float(idletime*100)/float(runtime)));
    printf("\n");

    quit_func(0);
}

int main(int argc, char* argv[])
{
    // Initialize timer and video systems
    if( SDL_Init( SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) == -1 ) 
    { 
        std::cerr << "SDL Initialization Failed: " << SDL_GetError() << std::endl;
        return 1; 
    }

    menu = new Menu(&cannonboard);

    bool loaded = false;

    // Load LayOut File
    if (argc == 3 && strcmp(argv[1], "-file") == 0)
    {
        if (trackloader.set_layout_track(argv[2]))
            loaded = roms.load_revb_roms(); 
    }
    // Load Roms Only
    else
    {
        loaded = roms.load_revb_roms();
    }

    //trackloader.set_layout_track("d:/temp.bin");
    //loaded = roms.load_revb_roms();

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

#ifdef COMPILE_SOUND_CODE
        audio.init();
#endif
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

        std::thread game (main_game_loop);
        std::thread sound (main_sound_loop);
        game.join();
        sound.join();
    }
    else
    {
        quit_func(1);
    }

    // Never Reached
    return 0;
}
