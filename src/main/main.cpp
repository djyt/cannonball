/***************************************************************************
    Cannonball Main Entry Point.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <cstring>
#include <iostream>

// SDL Library
#include <SDL.h>

// SDL Specific Code
#include "sdl2/timer.hpp"
#include "sdl2/input.hpp"

#include "video.hpp"

#include "romloader.hpp"
#include "trackloader.hpp"
#include "stdint.hpp"
#include "main.hpp"
#include "engine/outrun.hpp"
#include "frontend/config.hpp"
#include "frontend/menu.hpp"

#include "engine/oinputs.hpp"
#include "engine/ooutputs.hpp"
#include "engine/omusic.hpp"

// Direct X Haptic Support.
// Fine to include on non-windows builds as dummy functions used.
#include "directx/ffeedback.hpp"

// ------------------------------------------------------------------------------------------------
// Initialize Shared Variables
// ------------------------------------------------------------------------------------------------
using namespace cannonball;

int    cannonball::state       = STATE_BOOT;
double cannonball::frame_ms    = 0;
int    cannonball::frame       = 0;
bool   cannonball::tick_frame  = true;
int    cannonball::fps_counter = 0;

// ------------------------------------------------------------------------------------------------
// Main Variables and Pointers
// ------------------------------------------------------------------------------------------------
Audio cannonball::audio;
Menu* menu;
bool pause_engine;


// ------------------------------------------------------------------------------------------------

static void quit_func(int code)
{
    audio.stop_audio();
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

static void tick()
{
    frame++;

    // Non standard FPS: Determine whether to tick certain logic for the current frame.
    if (config.fps == 60)
        tick_frame = frame & 1;
    else if (config.fps == 120)
        tick_frame = (frame & 3) == 1;

    process_events();

    if (tick_frame)
        oinputs.tick(); // Do Controls
    oinputs.do_gear();        // Digital Gear

    switch (state)
    {
        case STATE_GAME:
        {
            if (input.has_pressed(Input::TIMER)) outrun.freeze_timer = !outrun.freeze_timer;
            if (input.has_pressed(Input::PAUSE)) pause_engine = !pause_engine;
            if (input.has_pressed(Input::MENU))  state = STATE_INIT_MENU;

            if (!pause_engine || input.has_pressed(Input::STEP))
            {
                outrun.tick(tick_frame);
                input.frame_done(); // Denote keys read
                osoundint.tick();
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
            menu->tick();
            input.frame_done();
            osoundint.tick();
            break;

        case STATE_INIT_MENU:
            oinputs.init();
            outrun.outputs->init();
            menu->init();
            state = STATE_MENU;
            break;
    }

    outrun.outputs->writeDigitalToConsole();
}

static void main_loop()
{
    // FPS Counter (If Enabled)
    Timer fps_count;
    int frame = 0;
    fps_count.start();

    // General Frame Timing
    bool vsync = config.video.vsync == 1 && video.supports_vsync();
    Timer frame_time;
    int t;                              // Actual timing of tick in ms as measured by SDL (ms)
    double deltatime  = 0;              // Time we want an entire frame to take (ms)
    int deltaintegral = 0;              // Integer version of above

    while (state != STATE_QUIT)
    {
        frame_time.start();
        // Tick Engine
        tick();

        // Draw SDL Video
        video.prepare_frame();
        video.render_frame();

        // Fill SDL Audio Buffer For Callback
        audio.tick();
        
        // Calculate Timings. Cap Frame Rate. Note this might be trumped by V-Sync
        if (!vsync)
        {
            deltatime += (frame_ms * audio.adjust_speed());
            deltaintegral = (int)deltatime;
            t = frame_time.get_ticks();
            
            if (t < deltatime)
                SDL_Delay((Uint32)(deltatime - t));

            deltatime -= deltaintegral;
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

    quit_func(0);
}

// Very (very) simple command line parser.
// Returns true if everything is ok to proceed with launching th engine.
static bool parse_command_line(int argc, char* argv[])
{
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-cfgfile") == 0 && i+1 < argc)
        {
            config.set_config_file(argv[i+1]);
        }
        else if (strcmp(argv[i], "-file") == 0 && i+1 < argc)
        {
            if (!trackloader.set_layout_track(argv[i+1]))
                return false;
        }
        else if (strcmp(argv[i], "-help") == 0)
        {
            std::cout << "Command Line Options:\n\n" <<
                         "-cfgfile: Location and name of config.xml\n" <<
                         "-file   : LayOut Editor track data to load\n" << std::endl;
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[])
{
    // Parse command line arguments (config file location, LayOut data) 
    bool ok = parse_command_line(argc, argv);

    if (ok)
    {
        config.load(); // Load config.XML file
        ok = roms.load_revb_roms(config.sound.fix_samples);
    }
    if (!ok)
    {
        quit_func(1);
        return 0;
    }

    // Load gamecontrollerdb.txt mappings
    if (SDL_GameControllerAddMappingsFromFile((config.data.res_path + "gamecontrollerdb.txt").c_str()) == -1)
        std::cout << "Unable to load controller mapping" << std::endl;

    // Initialize timer and video systems
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) == -1)
    {
        std::cerr << "SDL Initialization Failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Load patched widescreen tilemaps
    if (!omusic.load_widescreen_map(config.data.res_path))
        std::cout << "Unable to load widescreen tilemaps" << std::endl;

    // Initialize SDL Video
    config.set_fps(config.video.fps);
    if (!video.init(&roms, &config.video))
        quit_func(1);

    // Initialize SDL Audio
    audio.init();

    state = config.menu.enabled ? STATE_INIT_MENU : STATE_INIT_GAME;

    // Initalize SDL Controls
    input.init(config.controls.pad_id,
               config.controls.keyconfig, config.controls.padconfig, 
               config.controls.analog,    config.controls.axis, config.controls.asettings);

    if (config.controls.haptic) 
        config.controls.haptic = forcefeedback::init(config.controls.max_force, config.controls.min_force, config.controls.force_duration);
        
    // Populate menus
    menu = new Menu();
    menu->populate();
    main_loop();  // Loop until we quit the app

    // Never Reached
    return 0;
}
