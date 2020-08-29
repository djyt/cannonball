/***************************************************************************
    Front End Menu System.

    This file is part of Cannonball.
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

// Boost string prediction
#include <boost/algorithm/string/predicate.hpp>

#include "main.hpp"
#include "menu.hpp"
#include "setup.hpp"
#include "../utils.hpp"
#include "../cannonboard/interface.hpp"

#include "engine/ohud.hpp"
#include "engine/oinputs.hpp"
#include "engine/osprites.hpp"
#include "engine/ologo.hpp"
#include "engine/opalette.hpp"
#include "engine/otiles.hpp"

#include "frontend/cabdiag.hpp"
#include "frontend/ttrial.hpp"

// Logo Y Position
const static int16_t LOGO_Y = -60;

// Columns and rows available
const static uint16_t COLS = 40;
const static uint16_t ROWS = 28;

// Horizon Destination Position
const static uint16_t HORIZON_DEST = 0x3A0;

// ------------------------------------------------------------------------------------------------
// Text Labels for menus
// ------------------------------------------------------------------------------------------------

// Back Labels
const static char* ENTRY_BACK       = "BACK";

// Main Menu
const static char* ENTRY_PLAYGAME   = "PLAY GAME";
const static char* ENTRY_GAMEMODES  = "GAME MODES";
const static char* ENTRY_SETTINGS   = "SETTINGS";
const static char* ENTRY_ABOUT      = "ABOUT";
const static char* ENTRY_EXIT       = "EXIT";

// Game Modes Menu
const static char* ENTRY_ENHANCED   = "SET ENHANCED MODE";
const static char* ENTRY_ORIGINAL   = "SET ORIGINAL MODE";
const static char* ENTRY_CONT       = "CONTINUOUS MODE";
const static char* ENTRY_TIMETRIAL  = "TIME TRIAL MODE";

// Time Trial Menu
const static char* ENTRY_START      =  "START TIME TRIAL";
const static char* ENTRY_LAPS       =  "NO OF LAPS ";

// Continuous Menu
const static char* ENTRY_START_CONT = "START CONTINUOUS MODE";

// Settings Menu
const static char* ENTRY_VIDEO      = "VIDEO";
const static char* ENTRY_SOUND      = "SOUND";
const static char* ENTRY_CONTROLS   = "CONTROLS";
const static char* ENTRY_CANNONBOARD= "CANNONBOARD";
const static char* ENTRY_ENGINE     = "GAME ENGINE";
const static char* ENTRY_SCORES     = "CLEAR HISCORES";
const static char* ENTRY_SAVE       = "SAVE AND RETURN";

// CannonBoard Menu
const static char* ENTRY_C_INTERFACE= "INTERFACE DIAGNOSTICS";
const static char* ENTRY_C_INPUTS   = "INPUT TEST";
const static char* ENTRY_C_OUTPUTS  = "OUTPUT TEST";
const static char* ENTRY_C_CRT      = "CRT TEST";

// Video Menu
const static char* ENTRY_FPS           = "FRAME RATE ";
const static char* ENTRY_SCREENMODE    = "SCREEN MODE ";
const static char* ENTRY_WIDESCREEN    = "WIDESCREEN ";
const static char* ENTRY_HIRES         = "HIRES ";
const static char* ENTRY_CRT_OVERLAY   = "CRT OVERLAY... "; // JJP - Submenu for the CRT mask overlays
const static char* ENTRY_BLARGG_FILTER = "BLARGG CRT FILTER... "; // JJP - Submenu for the Blargg CRT filter settings

// CRT Overlay Menu
const static char* ENTRY_SCANLINES     = "SCANLINES ";
const static char* ENTRY_MASK          = "MASK  ";
const static char* ENTRY_MASKSTRENGTH  = "MASK STRENGTH ";
const static char* ENTRY_VIGNETTE      = "VIGNETTE ";

// Blargg filter sub-menu
const static char* ENTRY_BLARGG        = "BLARGG FILTERING ";
const static char* ENTRY_SATURATION    = "SATURATION ";
const static char* ENTRY_CONTRAST      = "CONTRAST ";
const static char* ENTRY_BRIGHTNESS    = "BRIGHTNESS ";
const static char* ENTRY_SHARPNESS     = "SHARPNESS ";
const static char* ENTRY_GAMMA         = "GAMMA ";

// Sound Menu
const static char* ENTRY_MUTE       = "SOUND ";
const static char* ENTRY_BGM        = "BGM VOL ";
const static char* ENTRY_SFX        = "SFX VOL ";
const static char* ENTRY_ADVERTISE  = "ADVERTISE SOUND ";
const static char* ENTRY_PREVIEWSND = "PREVIEW MUSIC ";
const static char* ENTRY_FIXSAMPLES = "FIX SAMPLES ";
const static char* ENTRY_MUSICTEST  = "MUSIC TEST";

// Controls Menu
const static char* ENTRY_GEAR       = "GEAR ";
const static char* ENTRY_ANALOG     = "ANALOG ";
const static char* ENTRY_REDEFJOY   = "REDEFINE GAMEPAD";
const static char* ENTRY_REDEFKEY   = "REDEFINE KEYS";
const static char* ENTRY_DSTEER     = "DIGITAL STEER SPEED ";
const static char* ENTRY_DPEDAL     = "DIGITAL PEDAL SPEED ";

// Game Engine Menu
const static char* ENTRY_TRACKS     = "TRACKS ";
const static char* ENTRY_TIME       = "TIME ";
const static char* ENTRY_TRAFFIC    = "TRAFFIC ";
const static char* ENTRY_OBJECTS    = "OBJECTS ";
const static char* ENTRY_PROTOTYPE  = "PROTOTYPE STAGE 1 ";
const static char* ENTRY_ATTRACT    = "NEW ATTRACT ";

// Music Test Menu
const static char* ENTRY_MUSIC1     = "MAGICAL SOUND SHOWER";
const static char* ENTRY_MUSIC2     = "PASSING BREEZE";
const static char* ENTRY_MUSIC3     = "SPLASH WAVE";
const static char* ENTRY_MUSIC4     = "LAST WAVE";

Menu::Menu(Interface* cannonboard)
{
    this->cannonboard = cannonboard;
    cabdiag = new CabDiag(cannonboard);
    ttrial  = new TTrial(config.ttrial.best_times);
}


Menu::~Menu(void)
{
    delete cabdiag;
    delete ttrial;
}

void Menu::populate()
{
    // Create Menus
    menu_main.push_back(ENTRY_PLAYGAME);
    menu_main.push_back(ENTRY_GAMEMODES);
    menu_main.push_back(ENTRY_SETTINGS);
    menu_main.push_back(ENTRY_ABOUT);
    menu_main.push_back(ENTRY_EXIT);

    menu_gamemodes.push_back(ENTRY_ENHANCED);
    menu_gamemodes.push_back(ENTRY_ORIGINAL);
    menu_gamemodes.push_back(ENTRY_CONT);
    menu_gamemodes.push_back(ENTRY_TIMETRIAL);
    menu_gamemodes.push_back(ENTRY_BACK);

    menu_cont.push_back(ENTRY_START_CONT);
    menu_cont.push_back(ENTRY_TRAFFIC);
    menu_cont.push_back(ENTRY_BACK);

    menu_timetrial.push_back(ENTRY_START);
    menu_timetrial.push_back(ENTRY_LAPS);
    menu_timetrial.push_back(ENTRY_TRAFFIC);
    menu_timetrial.push_back(ENTRY_BACK);

    menu_settings.push_back(ENTRY_VIDEO);
    #ifdef COMPILE_SOUND_CODE
    menu_settings.push_back(ENTRY_SOUND);
    #endif
    menu_settings.push_back(ENTRY_CONTROLS);
    if (config.cannonboard.enabled)
        menu_settings.push_back(ENTRY_CANNONBOARD);
    menu_settings.push_back(ENTRY_ENGINE);
    menu_settings.push_back(ENTRY_SCORES);
    menu_settings.push_back(ENTRY_SAVE);

    menu_cannonboard.push_back(ENTRY_C_INTERFACE);
    menu_cannonboard.push_back(ENTRY_C_INPUTS);
    menu_cannonboard.push_back(ENTRY_C_OUTPUTS);
    menu_cannonboard.push_back(ENTRY_C_CRT);
    menu_cannonboard.push_back(ENTRY_BACK);

    menu_video.push_back(ENTRY_FPS);
    menu_video.push_back(ENTRY_SCREENMODE);
    menu_video.push_back(ENTRY_WIDESCREEN);
    menu_video.push_back(ENTRY_HIRES);
    menu_video.push_back(ENTRY_CRT_OVERLAY); // JJP
    menu_video.push_back(ENTRY_BLARGG_FILTER); // JJP
    menu_video.push_back(ENTRY_BACK);

    menu_crt_overlay.push_back(ENTRY_SCANLINES);
    menu_crt_overlay.push_back(ENTRY_MASK);
    menu_crt_overlay.push_back(ENTRY_MASKSTRENGTH);
    menu_crt_overlay.push_back(ENTRY_VIGNETTE);
    menu_crt_overlay.push_back(ENTRY_BACK);

    // JJP - the following settings configure the CRT filtering
    menu_blargg_filter.push_back(ENTRY_BLARGG);
    menu_blargg_filter.push_back(ENTRY_SATURATION);
    menu_blargg_filter.push_back(ENTRY_CONTRAST);
    menu_blargg_filter.push_back(ENTRY_BRIGHTNESS);
    menu_blargg_filter.push_back(ENTRY_SHARPNESS);
    menu_blargg_filter.push_back(ENTRY_GAMMA);
    menu_blargg_filter.push_back(ENTRY_BACK);

    menu_sound.push_back(ENTRY_MUTE);
    //menu_sound.push_back(ENTRY_BGM);
    //menu_sound.push_back(ENTRY_SFX);
    menu_sound.push_back(ENTRY_ADVERTISE);
    menu_sound.push_back(ENTRY_PREVIEWSND);
    menu_sound.push_back(ENTRY_FIXSAMPLES);
    menu_sound.push_back(ENTRY_MUSICTEST);
    menu_sound.push_back(ENTRY_BACK);

    menu_controls.push_back(ENTRY_GEAR);
    if (input.gamepad) menu_controls.push_back(ENTRY_ANALOG);
    menu_controls.push_back(ENTRY_REDEFKEY);
    if (input.gamepad) menu_controls.push_back(ENTRY_REDEFJOY);
    menu_controls.push_back(ENTRY_DSTEER);
    menu_controls.push_back(ENTRY_DPEDAL);
    menu_controls.push_back(ENTRY_BACK);

    menu_engine.push_back(ENTRY_TRACKS);
    menu_engine.push_back(ENTRY_TIME);
    menu_engine.push_back(ENTRY_TRAFFIC);
    menu_engine.push_back(ENTRY_OBJECTS);
    menu_engine.push_back(ENTRY_PROTOTYPE);
    menu_engine.push_back(ENTRY_ATTRACT);
    menu_engine.push_back(ENTRY_BACK);

    menu_musictest.push_back(ENTRY_MUSIC1);
    menu_musictest.push_back(ENTRY_MUSIC2);
    menu_musictest.push_back(ENTRY_MUSIC3);
    menu_musictest.push_back(ENTRY_MUSIC4);
    menu_musictest.push_back(ENTRY_BACK);

    menu_about.push_back("CANNONBALL © CHRIS WHITE 2012,2020");
    menu_about.push_back("REASSEMBLER.BLOGSPOT.COM");
    menu_about.push_back("");
    menu_about.push_back("BLARGG INTEGRATION AND MULTI-");
    menu_about.push_back("THREADING BY JAMES PEARCE");
    menu_about.push_back("");
    menu_about.push_back("CANNONBALL IS FREE AND MAY NOT BE SOLD.");

    // Redefine menu text
    text_redefine.push_back("PRESS UP");
    text_redefine.push_back("PRESS DOWN");
    text_redefine.push_back("PRESS LEFT");
    text_redefine.push_back("PRESS RIGHT");
    text_redefine.push_back("PRESS ACCELERATE");
    text_redefine.push_back("PRESS BRAKE");
    text_redefine.push_back("PRESS GEAR");
    text_redefine.push_back("PRESS GEAR HIGH");
    text_redefine.push_back("PRESS START");
    text_redefine.push_back("PRESS COIN IN");
    text_redefine.push_back("PRESS MENU");
    text_redefine.push_back("PRESS VIEW CHANGE");
}

void Menu::init()
{
    // If we got a new high score on previous time trial, then save it!
    if (outrun.ttrial.new_high_score)
    {
        outrun.ttrial.new_high_score = false;
        ttrial->update_best_time();
    }

    outrun.select_course(false, config.engine.prototype != 0);
    video.enabled = true;
    video.sprite_layer->set_x_clip(false); // Stop clipping in wide-screen mode.
    video.sprite_layer->reset();
    video.clear_text_ram();
    video.tile_layer->restore_tiles();
    ologo.enable(LOGO_Y);

    // Setup palette, road and colours for background
    oroad.stage_lookup_off = 9;
    oinitengine.init_road_seg_master();
    opalette.setup_sky_palette();
    opalette.setup_ground_color();
    opalette.setup_road_centre();
    opalette.setup_road_stripes();
    opalette.setup_road_side();
    opalette.setup_road_colour();
    otiles.setup_palette_hud();

    oroad.init();
    oroad.road_ctrl = ORoad::ROAD_R0;
    oroad.horizon_set = 1;
    oroad.horizon_base = HORIZON_DEST + 0x100;
    oinitengine.rd_split_state = OInitEngine::SPLIT_NONE;
    oinitengine.car_increment = 0;
    oinitengine.change_width = 0;

    outrun.game_state = GS_INIT;

    set_menu(&menu_main);
    refresh_menu();

    // Reset audio, so we can play tones
    osoundint.has_booted = true;
    osoundint.init();
    cannonball::audio.clear_wav();

    frame = 0;
    message_counter = 0;

    if (config.cannonboard.enabled)
        display_message(cannonboard->started() ? "CANNONBOARD FOUND!" : "CANNONBOARD ERROR!");

    state = STATE_MENU;
}

void Menu::tick(Packet* packet)
{
    switch (state)
    {
        case STATE_MENU:
        case STATE_REDEFINE_KEYS:
        case STATE_REDEFINE_JOY:
            tick_ui();
            break;

        case STATE_DIAGNOSTICS:
            if (cabdiag->tick(packet))
            {
                init();
                set_menu(&menu_cannonboard);
                refresh_menu();
            }
            break;

        case STATE_TTRIAL:
            {
                int ttrial_state = ttrial->tick();

                if (ttrial_state == TTrial::INIT_GAME)
                {
                    cannonball::state = cannonball::STATE_INIT_GAME;
                    osoundint.queue_clear();
                }
                else if (ttrial_state == TTrial::BACK_TO_MENU)
                {
                    init();
                }
            }
            break;
    }
}

void Menu::tick_ui()
{
    // Skip odd frames at 60fps
    frame++;

    video.clear_text_ram();

    if (state == STATE_MENU)
    {
        tick_menu();
        draw_menu_options();
    }
    else if (state == STATE_REDEFINE_KEYS)
    {
        redefine_keyboard();
    }
    else if (state == STATE_REDEFINE_JOY)
    {
        redefine_joystick();
    }

    // Show messages
    if (message_counter > 0)
    {
        message_counter--;
        ohud.blit_text_new(0, 1, msg.c_str(), ohud.GREY);
    }

    // Shift horizon
    if (oroad.horizon_base > HORIZON_DEST)
    {
        oroad.horizon_base -= 60 / config.fps;
        if (oroad.horizon_base < HORIZON_DEST)
            oroad.horizon_base = HORIZON_DEST;
    }
    // Advance road
    else
    {
        uint32_t scroll_speed = (config.fps == 60) ? config.menu.road_scroll_speed : config.menu.road_scroll_speed << 1;

        if (oinitengine.car_increment < scroll_speed << 16)
            oinitengine.car_increment += (1 << 14);
        if (oinitengine.car_increment > scroll_speed << 16)
            oinitengine.car_increment = scroll_speed << 16;
        uint32_t result = 0x12F * (oinitengine.car_increment >> 16);
        oroad.road_pos_change = result;
        oroad.road_pos += result;
        if (oroad.road_pos >> 16 > ROAD_END) // loop to beginning of track data
            oroad.road_pos = 0;
        oinitengine.update_road();
        oinitengine.set_granular_position();
        oroad.road_width_bak = oroad.road_width >> 16; 
        oroad.car_x_bak = -oroad.road_width_bak; 
        oinitengine.car_x_pos = oroad.car_x_bak;
    }

    // Do Animations at 30 fps
    if (config.fps != 60 || (frame & 1) == 0)
    {
        ologo.tick();
        osprites.sprite_copy();
        osprites.update_sprites();
    }

    // Draw FPS
    if (config.video.fps_count)
        ohud.draw_fps_counter(cannonball::fps_counter);

    oroad.tick();
}

void Menu::draw_menu_options()
{
    int8_t x = 0;

    // Find central column in screen. 
    int8_t y = 13 + ((ROWS - 13) >> 1) - ((menu_selected->size() * 2) >> 1);

    for (int i = 0; i < (int) menu_selected->size(); i++)
    {
        std::string s = menu_selected->at(i);

        // Centre the menu option
        x = 20 - (s.length() >> 1);
        ohud.blit_text_new(x, y, s.c_str(), ohud.GREEN);

        if (!is_text_menu)
        {
            // Draw minicar
            if (i == cursor)
                video.write_text32(ohud.translate(x - 3, y), roms.rom0.read32(TILES_MINICARS1));
            // Erase minicar from this position
            else
                video.write_text32(ohud.translate(x - 3, y), 0x20202020);
        }
        y += 2;
    }
}

// Draw a single line of text
void Menu::draw_text(std::string s)
{
    // Centre text
    int8_t x = 20 - (s.length() >> 1);

    // Find central column in screen. 
    int8_t y = 13 + ((ROWS - 13) >> 1) - 1;

    ohud.blit_text_new(x, y, s.c_str(), ohud.GREEN);
}

#define SELECTED(string) boost::starts_with(OPTION, string)

void Menu::tick_menu()
{
    // Tick Controls
    if (input.has_pressed(Input::DOWN) || oinputs.is_analog_r())
    {
        osoundint.queue_sound(sound::BEEP1);

        if (++cursor >= (int16_t) menu_selected->size())
            cursor = 0;
    }
    else if (input.has_pressed(Input::UP) || oinputs.is_analog_l())
    {
        osoundint.queue_sound(sound::BEEP1);

        if (--cursor < 0)
            cursor = menu_selected->size() - 1;
    }
    else if (input.has_pressed(Input::ACCEL) || input.has_pressed(Input::START) || oinputs.is_analog_select())
    {
        // Get option that was selected
        const char* OPTION = menu_selected->at(cursor).c_str();

        if (menu_selected == &menu_main)
        {
            if (SELECTED(ENTRY_PLAYGAME))
            {
                start_game(Outrun::MODE_ORIGINAL);
                //cabdiag->set(CabDiag::STATE_MOTORT);
                //state = STATE_DIAGNOSTICS;
                return;
            }
            else if (SELECTED(ENTRY_GAMEMODES))
                set_menu(&menu_gamemodes);
            else if (SELECTED(ENTRY_SETTINGS))
                set_menu(&menu_settings);
            else if (SELECTED(ENTRY_ABOUT))
                set_menu(&menu_about);
            else if (SELECTED(ENTRY_EXIT))
                cannonball::state = cannonball::STATE_QUIT;
        }
        else if (menu_selected == &menu_gamemodes)
        {
            if (SELECTED(ENTRY_ENHANCED))
                start_game(Outrun::MODE_ORIGINAL, 1);
            else if (SELECTED(ENTRY_ORIGINAL))
                start_game(Outrun::MODE_ORIGINAL, 2);
            else if (SELECTED(ENTRY_CONT))
                set_menu(&menu_cont);
            else if (SELECTED(ENTRY_TIMETRIAL))
                set_menu(&menu_timetrial);
            else if (SELECTED(ENTRY_BACK))
                set_menu(&menu_main);
        }
        else if (menu_selected == &menu_cont)
        {
            if (SELECTED(ENTRY_START_CONT))
            {
                config.save(FILENAME_CONFIG);
                outrun.custom_traffic = config.cont_traffic;
                start_game(Outrun::MODE_CONT);
            }
            else if (SELECTED(ENTRY_TRAFFIC))
            {
                if (++config.cont_traffic > TTrial::MAX_TRAFFIC)
                    config.cont_traffic = 0;
            }
            else if (SELECTED(ENTRY_BACK))
                set_menu(&menu_gamemodes);
        }
        else if (menu_selected == &menu_timetrial)
        {
            if (SELECTED(ENTRY_START))
            {
                if (check_jap_roms())
                {
                    config.save(FILENAME_CONFIG);
                    state = STATE_TTRIAL;
                    ttrial->init();
                }
            }
            else if (SELECTED(ENTRY_LAPS))
            {
                if (++config.ttrial.laps > TTrial::MAX_LAPS)
                    config.ttrial.laps = 1;
            }
            else if (SELECTED(ENTRY_TRAFFIC))
            {
                if (++config.ttrial.traffic > TTrial::MAX_TRAFFIC)
                    config.ttrial.traffic = 0;
            }
            else if (SELECTED(ENTRY_BACK))
                set_menu(&menu_gamemodes);
        }
        else if (menu_selected == &menu_settings)
        {
            if (SELECTED(ENTRY_CANNONBOARD))
                set_menu(&menu_cannonboard);
            else if (SELECTED(ENTRY_VIDEO))
                set_menu(&menu_video);
            else if (SELECTED(ENTRY_SOUND))
                set_menu(&menu_sound);
            else if (SELECTED(ENTRY_CONTROLS))
            {
                if (input.gamepad)
                    display_message("GAMEPAD FOUND");
                set_menu(&menu_controls);
            }
            else if (SELECTED(ENTRY_ENGINE))
                set_menu(&menu_engine);
            else if (SELECTED(ENTRY_SCORES))
            {
                if (config.clear_scores())
                    display_message("SCORES CLEARED");
                else
                    display_message("NO SAVED SCORES FOUND!");
            }
            else if (SELECTED(ENTRY_SAVE))
            {
                if (config.save(FILENAME_CONFIG))
                    display_message("SETTINGS SAVED");
                else
                    display_message("ERROR SAVING SETTINGS!");
                set_menu(&menu_main);
            }
        }
        else if (menu_selected == &menu_about)
        {
            set_menu(&menu_main);
        }
        else if (menu_selected == &menu_cannonboard)
        {
            if (SELECTED(ENTRY_BACK))
                set_menu(&menu_settings);
            else if (SELECTED(ENTRY_C_INTERFACE))
            {
                cabdiag->set(CabDiag::STATE_INTERFACE);
                state = STATE_DIAGNOSTICS; return;
            }
            else if (SELECTED(ENTRY_C_INPUTS))
            {
                cabdiag->set(CabDiag::STATE_INPUT);
                state = STATE_DIAGNOSTICS; return;
            }
            else if (SELECTED(ENTRY_C_OUTPUTS))
            {
                cabdiag->set(CabDiag::STATE_OUTPUT);
                state = STATE_DIAGNOSTICS; return;
            }
            else if (SELECTED(ENTRY_C_CRT))
            {
                cabdiag->set(CabDiag::STATE_CRT);
                state = STATE_DIAGNOSTICS; return;
            }
        }
        else if (menu_selected == &menu_video)
        {
            if (SELECTED(ENTRY_FPS))
            {
                if (++config.video.fps > 2)
                    config.video.fps = 0;
                config.set_fps(config.video.fps);
            }
            else if (SELECTED(ENTRY_SCREENMODE))
            {
                int max_scale = (config.video.hires ? 2 : 4);
                if (config.video.mode == video_settings_t::MODE_FULL) {
                    // switch to window mode with 1x scale
                    config.video.mode = video_settings_t::MODE_WINDOW;
                    config.video.scale = 1;
                } else if (config.video.scale < max_scale) {
                    // increment window scalining
                    config.video.scale++;
                } else {
                    // wrap back to full screen
                    config.video.mode = video_settings_t::MODE_FULL;
                    config.video.scale = 1;
                }
                restart_video();
            }
            else if (SELECTED(ENTRY_WIDESCREEN))
            {
                config.video.widescreen = !config.video.widescreen;
                restart_video();
            }
            else if (SELECTED(ENTRY_HIRES))
            {
                config.video.hires = !config.video.hires;
                if (config.video.hires) {
                    if (config.video.scale > 1) config.video.scale >>= 1;
                } else config.video.scale <<= 1;

                restart_video();
                video.sprite_layer->set_x_clip(false);
            }
            else if (SELECTED(ENTRY_CRT_OVERLAY))
                set_menu(&menu_crt_overlay);
            else if (SELECTED(ENTRY_BLARGG_FILTER))
                set_menu(&menu_blargg_filter);
            else if (SELECTED(ENTRY_BACK))
                set_menu(&menu_settings);
        }
        else if (menu_selected == &menu_crt_overlay)
        {
            // JJP - CRT mask overlay related settings and options
            if (SELECTED(ENTRY_SCANLINES))
            {
                config.video.scanlines += 10;
                if (config.video.scanlines > 100)
                    config.video.scanlines = 0;
                restart_video();
            }
            else if (SELECTED(ENTRY_MASK))
            {
                // defines the type of mask shown
                config.video.mask += 1;
                if (config.video.mask > 7)
                    config.video.mask = 0;
                restart_video();
            }
            else if (SELECTED(ENTRY_MASKSTRENGTH))
            {
                // defines overlay stregth of the mask
                config.video.mask_strength += 5;
                if (config.video.mask_strength > 35)
                    config.video.mask_strength = 0;
                restart_video();
            }
            else if (SELECTED(ENTRY_VIGNETTE)) // JJP - simple vignette post-process filter
            {
                config.video.vignette += 10;
                if (config.video.vignette > 100)
                    config.video.vignette = 0;
                restart_video();
            }
            else if (SELECTED(ENTRY_BACK))
                set_menu(&menu_video);
        }
        else if (menu_selected == &menu_blargg_filter)
        {
            // JJP - Blargg CRT filtering settings
            if (SELECTED(ENTRY_BLARGG)) // off/componsite/s-video/rgb
            {
                if (++config.video.blargg > video_settings_t::BLARGG_RGB)
                    config.video.blargg = video_settings_t::BLARGG_DISABLE;
                restart_video();
            }
            else if (SELECTED(ENTRY_SATURATION))
            {
                config.video.saturation += 10;
                if (config.video.saturation > 50)
                    config.video.saturation = -50;
                restart_video();
            }
            else if (SELECTED(ENTRY_CONTRAST))
            {
                config.video.contrast += 10;
                if (config.video.contrast > 50)
                    config.video.contrast = -50;
                restart_video();
            }
            else if (SELECTED(ENTRY_BRIGHTNESS))
            {
                config.video.brightness += 10;
                if (config.video.brightness > 50)
                    config.video.brightness = -50;
                restart_video();
            }
            else if (SELECTED(ENTRY_SHARPNESS))
            {
                config.video.sharpness += 10;
                if (config.video.sharpness > 50)
                    config.video.sharpness = -50;
                restart_video();
            }
            else if (SELECTED(ENTRY_GAMMA))
            {
                config.video.gamma += 50;
                if (config.video.gamma > 300)
                    config.video.gamma = -300;
                restart_video();
            }
            else if (SELECTED(ENTRY_BACK))
                set_menu(&menu_video);
        }
        else if (menu_selected == &menu_sound)
        {
            if (SELECTED(ENTRY_MUTE))
            {
                config.sound.enabled = !config.sound.enabled;
                #ifdef COMPILE_SOUND_CODE
                if (config.sound.enabled)
                    cannonball::audio.start_audio();
                else
                    cannonball::audio.stop_audio();
                #endif
            }
            else if (SELECTED(ENTRY_ADVERTISE))
                config.sound.advertise = !config.sound.advertise;
            else if (SELECTED(ENTRY_PREVIEWSND))
                config.sound.preview = !config.sound.preview;
            else if (SELECTED(ENTRY_FIXSAMPLES))
            {
                int rom_type = !config.sound.fix_samples;
                if (roms.load_pcm_rom(rom_type == 1))
                {
                    config.sound.fix_samples = rom_type;
                    display_message(rom_type == 1 ? "FIXED SAMPLES LOADED" : "ORIGINAL SAMPLES LOADED");
                }
                else
                {
                    display_message(rom_type == 1 ? "CANT LOAD FIXED SAMPLES" : "CANT LOAD ORIGINAL SAMPLES");
                }
            }
            else if (SELECTED(ENTRY_MUSICTEST))
                set_menu(&menu_musictest);
            else if (SELECTED(ENTRY_BACK))
                set_menu(&menu_settings);
        }
        else if (menu_selected == &menu_controls)
        {
            if (SELECTED(ENTRY_GEAR))
            {
                if (++config.controls.gear > config.controls.GEAR_AUTO)
                    config.controls.gear = config.controls.GEAR_BUTTON;
            }
            else if (SELECTED(ENTRY_ANALOG))
            {
                if (++config.controls.analog == 3)
                    config.controls.analog = 0;
                input.analog = config.controls.analog;
            }
            else if (SELECTED(ENTRY_REDEFKEY))
            {
                display_message("PRESS MENU TO END AT ANY STAGE");
                state = STATE_REDEFINE_KEYS;
                redef_state = 0;
                input.key_press = -1;
            }
            else if (SELECTED(ENTRY_REDEFJOY))
            {
                display_message("PRESS MENU TO END AT ANY STAGE");
                state = STATE_REDEFINE_JOY;
                redef_state = config.controls.analog == 1 ? 2 : 0; // Ignore pedals when redefining analog
                input.joy_button = -1;
            }
            else if (SELECTED(ENTRY_DSTEER))
            {
                if (++config.controls.steer_speed > 9)
                    config.controls.steer_speed = 1;
            }
            else if (SELECTED(ENTRY_DPEDAL))
            {
                if (++config.controls.pedal_speed > 9)
                    config.controls.pedal_speed = 1;
            }
            else if (SELECTED(ENTRY_BACK))
                set_menu(&menu_settings);
        }
        else if (menu_selected == &menu_engine)
        {
            if (SELECTED(ENTRY_TRACKS))
            {
                config.engine.jap = !config.engine.jap;
            }
            else if (SELECTED(ENTRY_TIME))
            {
                if (config.engine.dip_time == 3)
                {
                    if (!config.engine.freeze_timer)
                        config.engine.freeze_timer = 1;
                    else
                    {
                        config.engine.dip_time = 0;
                        config.engine.freeze_timer = 0;
                    }
                }
                else
                    config.engine.dip_time++;
            }
            else if (SELECTED(ENTRY_TRAFFIC))
            {
                if (config.engine.dip_traffic == 3)
                {
                    if (!config.engine.disable_traffic)
                        config.engine.disable_traffic = 1;
                    else
                    {
                        config.engine.dip_traffic = 0;
                        config.engine.disable_traffic = 0;
                    }
                }
                else
                    config.engine.dip_traffic++;
            }
            else if (SELECTED(ENTRY_OBJECTS))
                config.engine.level_objects = !config.engine.level_objects;
            else if (SELECTED(ENTRY_PROTOTYPE))
                config.engine.prototype = !config.engine.prototype;
            else if (SELECTED(ENTRY_ATTRACT))
                config.engine.new_attract ^= 1;
            if (SELECTED(ENTRY_BACK))
                set_menu(&menu_settings);
        }
        else if (menu_selected == &menu_musictest)
        {
            if (SELECTED(ENTRY_MUSIC1))
                osoundint.queue_sound(sound::MUSIC_MAGICAL);
            else if (SELECTED(ENTRY_MUSIC2))
                osoundint.queue_sound(sound::MUSIC_BREEZE);
            else if (SELECTED(ENTRY_MUSIC3))
                osoundint.queue_sound(sound::MUSIC_SPLASH);
            else if (SELECTED(ENTRY_MUSIC4))
                osoundint.queue_sound(sound::MUSIC_LASTWAVE);
            else if (SELECTED(ENTRY_BACK)) {
//                osoundint.queue_sound(sound::FM_RESET);
                set_menu(&menu_sound);
            }
        }
        else
            set_menu(&menu_main);

        osoundint.queue_sound(sound::BEEP1);
        refresh_menu();
    }
}

// Set Current Menu
void Menu::set_menu(std::vector<std::string> *menu)
{
    menu_selected = menu;
    cursor = 0;

    is_text_menu = (menu == &menu_about);
}

// Refresh menu options with latest config data
void Menu::refresh_menu()
{
    int16_t cursor_backup = cursor;
    std::string s;

    for (cursor = 0; cursor < (int) menu_selected->size(); cursor++)
    {
        // Get option that was selected
        const char* OPTION = menu_selected->at(cursor).c_str();

        if (menu_selected == &menu_timetrial)
        {
            if (SELECTED(ENTRY_LAPS))
                set_menu_text(ENTRY_LAPS, Utils::to_string(config.ttrial.laps));
            else if (SELECTED(ENTRY_TRAFFIC))
                set_menu_text(ENTRY_TRAFFIC, config.ttrial.traffic == 0 ? "DISABLED" : Utils::to_string(config.ttrial.traffic));
        }
        else if (menu_selected == &menu_cont)
        {
            if (SELECTED(ENTRY_TRAFFIC))
                set_menu_text(ENTRY_TRAFFIC, config.cont_traffic == 0 ? "DISABLED" : Utils::to_string(config.cont_traffic));
        }
        else if (menu_selected == &menu_video)
        {
            if (SELECTED(ENTRY_FPS))
            {
                if (config.video.fps == 0)      s = "30 FPS";
                else if (config.video.fps == 1) s = "ORIGINAL";
                else if (config.video.fps == 2) s = "60 FPS";
                set_menu_text(ENTRY_FPS, s);
            }
            else if (SELECTED(ENTRY_SCREENMODE))
            {
                if (config.video.mode == video_settings_t::MODE_FULL) s = "FULLSCREEN";
                else s = "WINDOW " + Utils::to_string(config.video.scale) + "x";
//                else if (config.video.mode == video_settings_t::MODE_STRETCH) s = "STRETCH ";
                set_menu_text(ENTRY_SCREENMODE, s);
            }
            else if (SELECTED(ENTRY_WIDESCREEN))
                set_menu_text(ENTRY_WIDESCREEN, config.video.widescreen ? "ON" : "OFF");
//            else if (SELECTED(ENTRY_SCALE))
//                set_menu_text(ENTRY_SCALE, Utils::to_string(config.video.scale) + "X");
            else if (SELECTED(ENTRY_HIRES))
                set_menu_text(ENTRY_HIRES, config.video.hires ? "ON" : "OFF");
        }
        else if (menu_selected == &menu_crt_overlay)
        {
            // Screen feedback for selected options for the blargg filtering
            if (SELECTED(ENTRY_MASK))
                set_menu_text(ENTRY_MASK, config.video.mask ? Utils::to_string(config.video.mask) : "OFF");
            else if (SELECTED(ENTRY_MASKSTRENGTH))
                set_menu_text(ENTRY_MASKSTRENGTH, config.video.mask_strength ? Utils::to_string(config.video.mask_strength) + "%" : "OFF");
            else if (SELECTED(ENTRY_SCANLINES))
                set_menu_text(ENTRY_SCANLINES, config.video.scanlines ? Utils::to_string(config.video.scanlines) +"%": "OFF");
            else if (SELECTED(ENTRY_VIGNETTE))
                set_menu_text(ENTRY_VIGNETTE, config.video.vignette ? Utils::to_string(config.video.vignette) +"%": "OFF");
        }
        else if (menu_selected == &menu_blargg_filter)
        {
            // Screen feedback for selected options for the blargg filtering
            if (SELECTED(ENTRY_BLARGG))
            {
                if (config.video.blargg == video_settings_t::BLARGG_DISABLE)        s = "OFF";
                else if (config.video.blargg == video_settings_t::BLARGG_COMPOSITE) s = "COMPOSITE";
                else if (config.video.blargg == video_settings_t::BLARGG_SVIDEO)    s = "S-VIDEO";
                else if (config.video.blargg == video_settings_t::BLARGG_RGB)       s = "ARCADE RGB";
                else if (config.video.blargg == video_settings_t::BLARGG_MONO)      s = "MONO";
                set_menu_text(ENTRY_BLARGG, s);
            }
            else if (SELECTED(ENTRY_SATURATION))
                set_menu_text(ENTRY_SATURATION, Utils::to_string(config.video.saturation));
            else if (SELECTED(ENTRY_CONTRAST))
                set_menu_text(ENTRY_CONTRAST, Utils::to_string(config.video.contrast));
            else if (SELECTED(ENTRY_BRIGHTNESS))
                set_menu_text(ENTRY_BRIGHTNESS, Utils::to_string(config.video.brightness));
            else if (SELECTED(ENTRY_SHARPNESS))
                set_menu_text(ENTRY_SHARPNESS, Utils::to_string(config.video.sharpness));
            else if (SELECTED(ENTRY_GAMMA))
                set_menu_text(ENTRY_GAMMA, Utils::to_string(config.video.gamma));
        }
        else if (menu_selected == &menu_sound)
        {
            if (SELECTED(ENTRY_MUTE))
                set_menu_text(ENTRY_MUTE, config.sound.enabled ? "ON" : "OFF");
            else if (SELECTED(ENTRY_ADVERTISE))
                set_menu_text(ENTRY_ADVERTISE, config.sound.advertise ? "ON" : "OFF");
            else if (SELECTED(ENTRY_PREVIEWSND))
                set_menu_text(ENTRY_PREVIEWSND, config.sound.preview ? "ON" : "OFF");
            else if (SELECTED(ENTRY_FIXSAMPLES))
                set_menu_text(ENTRY_FIXSAMPLES, config.sound.fix_samples ? "ON" : "OFF");
        }
        else if (menu_selected == &menu_controls)
        {
            if (SELECTED(ENTRY_GEAR))
            {
                if (config.controls.gear == config.controls.GEAR_BUTTON)        s = "MANUAL";
                else if (config.controls.gear == config.controls.GEAR_PRESS)    s = "MANUAL CABINET";
                else if (config.controls.gear == config.controls.GEAR_SEPARATE) s = "MANUAL 2 BUTTONS";
                else if (config.controls.gear == config.controls.GEAR_AUTO)     s = "AUTOMATIC";
                set_menu_text(ENTRY_GEAR, s);
            }
            else if (SELECTED(ENTRY_ANALOG))
            {
                if (config.controls.analog == 0)      s = "OFF";
                else if (config.controls.analog == 1) s = "ON";
                else if (config.controls.analog == 2) s = "ON WHEEL ONLY";
                set_menu_text(ENTRY_ANALOG, s);
            }
            else if (SELECTED(ENTRY_DSTEER))
                set_menu_text(ENTRY_DSTEER, Utils::to_string(config.controls.steer_speed));
            else if (SELECTED(ENTRY_DPEDAL))
                set_menu_text(ENTRY_DPEDAL, Utils::to_string(config.controls.pedal_speed));
        }
        else if (menu_selected == &menu_engine)
        {
            if (SELECTED(ENTRY_TRACKS))
                set_menu_text(ENTRY_TRACKS, config.engine.jap ? "JAPAN" : "WORLD");
            else if (SELECTED(ENTRY_TIME))
            {
                if (config.engine.freeze_timer)       s = "INFINITE";
                else if (config.engine.dip_time == 0) s = "EASY";
                else if (config.engine.dip_time == 1) s = "NORMAL";
                else if (config.engine.dip_time == 2) s = "HARD";
                else if (config.engine.dip_time == 3) s = "HARDEST";          
                set_menu_text(ENTRY_TIME, s);
            }
            else if (SELECTED(ENTRY_TRAFFIC))
            {
                if (config.engine.disable_traffic)       s = "DISABLED";
                else if (config.engine.dip_traffic == 0) s = "EASY";
                else if (config.engine.dip_traffic == 1) s = "NORMAL";
                else if (config.engine.dip_traffic == 2) s = "HARD";
                else if (config.engine.dip_traffic == 3) s = "HARDEST";          
                set_menu_text(ENTRY_TRAFFIC, s);
            }
            else if (SELECTED(ENTRY_OBJECTS))
                set_menu_text(ENTRY_OBJECTS, config.engine.level_objects ? "ENHANCED" : "ORIGINAL");
            else if (SELECTED(ENTRY_PROTOTYPE))
                set_menu_text(ENTRY_PROTOTYPE, config.engine.prototype ? "ON" : "OFF");
            else if (SELECTED(ENTRY_ATTRACT))
                set_menu_text(ENTRY_ATTRACT, config.engine.new_attract ? "ON" : "OFF");

        }
    }
    cursor = cursor_backup;
}

// Append Menu Text For A Particular Menu Entry
void Menu::set_menu_text(std::string s1, std::string s2)
{
    s1.append(s2);
    menu_selected->erase(menu_selected->begin() + cursor);
    menu_selected->insert(menu_selected->begin() + cursor, s1);
}

void Menu::redefine_keyboard()
{
    if (redef_state == 7 && config.controls.gear != config.controls.GEAR_SEPARATE) // Skip redefine of second gear press
        redef_state++;

    switch (redef_state)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            if (input.has_pressed(Input::MENU))
            {
                message_counter = 0;
                state = STATE_MENU;
            }
            else
            {
                draw_text(text_redefine.at(redef_state));
                if (input.key_press != -1)
                {
                    config.controls.keyconfig[redef_state] = input.key_press;
                    redef_state++;
                    input.key_press = -1;
                }
            }
            break;

        case 12:
            state = STATE_MENU;
            break;
    }
}

void Menu::redefine_joystick()
{
    if (redef_state == 3 && config.controls.gear != config.controls.GEAR_SEPARATE) // Skip redefine of second gear press
        redef_state++;

    switch (redef_state)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            if (input.has_pressed(Input::MENU))
            {
                message_counter = 0;
                state = STATE_MENU;
            }
            else
            {
                draw_text(text_redefine.at(redef_state + 4));
                if (input.joy_button != -1)
                {
                    config.controls.padconfig[redef_state] = input.joy_button;
                    redef_state++;
                    input.joy_button = -1;
                }
            }
            break;

        case 8:
            state = STATE_MENU;
            break;
    }
}

// Display a contextual message in the top left of the screen
void Menu::display_message(std::string s)
{
    msg = s;
    message_counter = MESSAGE_TIME * config.fps;
}

bool Menu::check_jap_roms()
{
    if (config.engine.jap && !roms.load_japanese_roms())
    {
        display_message("JAPANESE ROMSET NOT FOUND");
        return false;
    }
    return true;
}

// Reinitalize Video, and stop audio to avoid crackles
void Menu::restart_video()
{
    #ifdef COMPILE_SOUND_CODE
    if (config.sound.enabled) cannonball::audio.pause_audio();
//        cannonball::audio.stop_audio();
    SDL_Delay(8); // await call-back
    #endif
    video.disable();
    video.init(&roms, &config.video);
    #ifdef COMPILE_SOUND_CODE
//    osoundint.init();
    if (config.sound.enabled) cannonball::audio.resume_audio();
//        cannonball::audio.start_audio();
    #endif
}

void Menu::start_game(int mode, int settings)
{
    // Enhanced Settings
    if (settings == 1)
    {
        if (!config.video.hires)
        {
            if (config.video.scale > 1)
                config.video.scale >>= 1;
        }

        if (!config.sound.fix_samples)
        {
            if (roms.load_pcm_rom(true))
                config.sound.fix_samples = 1;
        }

        config.set_fps(config.video.fps = 2);
        config.video.widescreen     = 1;
        config.video.hires          = 1;
        config.engine.level_objects = 1;
        config.engine.new_attract   = 1;
        config.engine.fix_bugs      = 1;
        config.engine.fix_timer     = 1;
        config.sound.preview        = 1;
        restart_video();
    }
    // Original Settings
    else if (settings == 2)
    {
        if (config.video.hires)
        {
            config.video.scale <<= 1;
        }

        if (config.sound.fix_samples)
        {
            if (roms.load_pcm_rom(false))
                config.sound.fix_samples = 0;
        }

        config.set_fps(config.video.fps = 1);
        config.video.widescreen     = 0;
        config.video.hires          = 0;
        config.engine.level_objects = 0;
        config.engine.new_attract   = 0;
        config.engine.fix_bugs      = 0;
        config.sound.preview        = 0;

        restart_video();
    }
    // Otherwise, use whatever is already setup...
    else
    {
        config.engine.fix_bugs = config.engine.fix_bugs_backup;
    }

    if (check_jap_roms())
    {
        outrun.cannonball_mode = mode;
        cannonball::state = cannonball::STATE_INIT_GAME;
        osoundint.queue_clear();
    }
}
