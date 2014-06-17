/***************************************************************************
    Cannonball Main Entry Point.
    
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

// SDL Specific Code
#include "sdl/timer.hpp"
#include "sdl/input.hpp"
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

// Direct X Haptic Support.
// Fine to include on non-windows builds as dummy functions used.
#include "directx/ffeedback.hpp"

// Initialize Shared Variables
using namespace cannonball;

int    cannonball::state       = STATE_BOOT;
double cannonball::frame_ms    = 0;
int    cannonball::frame       = 0;
bool   cannonball::tick_frame  = true;
int    cannonball::fps_counter = 0;

#ifdef COMPILE_SOUND_CODE
Audio cannonball::audio;
#endif

Menu* menu;
Interface cannonboard;

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
    Packet* packet = config.cannonboard.enabled ? &cannonboard.get_packet() : NULL;

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
                osoundint.tick();
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
            osoundint.tick();
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

    // Draw SDL Video
    video.draw_frame();  
}

static void main_loop()
{
    // FPS Counter (If Enabled)
    Timer fps_count;
    int frame = 0;
    fps_count.start();

    // General Frame Timing
    Timer frame_time;
    int t;
    double deltatime  = 0;
    int deltaintegral = 0;

    while (state != STATE_QUIT)
    {
        frame_time.start();
        tick();
        #ifdef COMPILE_SOUND_CODE
        deltatime += (frame_ms * audio.adjust_speed());
        #else
        deltatime += frame_ms;
        #endif
        deltaintegral  = (int) deltatime;
        t = frame_time.get_ticks();

        // Cap Frame Rate: Sleep Remaining Frame Time
        if (t < deltatime)
        {
            SDL_Delay((Uint32) (deltatime - t));
        }
        
        deltatime -= deltaintegral;

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

        //Set the window caption 
        SDL_WM_SetCaption( "Cannonball", NULL ); 

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
        main_loop();  // Loop until we quit the app
    }
    else
    {
        quit_func(1);
    }

    // Never Reached
    return 0;
}
