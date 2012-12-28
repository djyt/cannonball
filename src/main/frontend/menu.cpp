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

#include "engine/osprites.hpp"
#include "engine/ologo.hpp"
#include "engine/opalette.hpp"

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
const static char* ENTRY_SETTINGS   = "SETTINGS";
const static char* ENTRY_ABOUT      = "ABOUT";
const static char* ENTRY_EXIT       = "EXIT";

// Settings Menu
const static char* ENTRY_VIDEO      = "VIDEO";
const static char* ENTRY_SOUND      = "SOUND";
const static char* ENTRY_CONTROLS   = "CONTROLS";
const static char* ENTRY_ENGINE     = "GAME ENGINE";
const static char* ENTRY_SCORES     = "CLEAR HISCORES";

// Video Menu
const static char* ENTRY_FPS        = "FRAME RATE ";

// Sound Menu
const static char* ENTRY_MUTE       = "SOUND ";
const static char* ENTRY_BGM        = "BGM VOL ";
const static char* ENTRY_SFX        = "SFX VOL ";
const static char* ENTRY_ADVERTISE  = "ADVERTISE SOUND ";

// Controls Menu
const static char* ENTRY_GEAR       = "GEAR ";
const static char* ENTRY_DSTEER     = "DIGITAL STEER SPEED ";
const static char* ENTRY_DPEDAL     = "DIGITAL PEDAL SPEED ";

// Game Engine Menu
const static char* ENTRY_TIME       = "TIME ";
const static char* ENTRY_TRAFFIC    = "TRAFFIC ";
const static char* ENTRY_OBJECTS    = "OBJECTS ";

Menu::Menu(void)
{
    // Create Menus
    menu_main.push_back(ENTRY_PLAYGAME);
    menu_main.push_back(ENTRY_SETTINGS);
    menu_main.push_back(ENTRY_ABOUT);
    menu_main.push_back(ENTRY_EXIT);

    menu_settings.push_back(ENTRY_VIDEO);
    menu_settings.push_back(ENTRY_SOUND);
    menu_settings.push_back(ENTRY_CONTROLS);
    menu_settings.push_back(ENTRY_ENGINE);
    menu_settings.push_back(ENTRY_SCORES);
    menu_settings.push_back(ENTRY_BACK);

    menu_video.push_back(ENTRY_FPS);
    menu_video.push_back(ENTRY_BACK);

    menu_sound.push_back(ENTRY_MUTE);
    //menu_sound.push_back(ENTRY_BGM);
    //menu_sound.push_back(ENTRY_SFX);
    menu_sound.push_back(ENTRY_ADVERTISE);
    menu_sound.push_back(ENTRY_BACK);

    menu_controls.push_back(ENTRY_GEAR);
    menu_controls.push_back(ENTRY_DSTEER);
    menu_controls.push_back(ENTRY_DPEDAL);
    menu_controls.push_back(ENTRY_BACK);

    menu_engine.push_back(ENTRY_TIME);
    menu_engine.push_back(ENTRY_TRAFFIC);
    menu_engine.push_back(ENTRY_OBJECTS);
    menu_engine.push_back(ENTRY_BACK);

    menu_about.push_back("CANNONBALL © CHRIS WHITE 2012");
    menu_about.push_back("REASSEMBLER.BLOGSPOT.COM");
    menu_about.push_back(" ");
    menu_about.push_back("CANNONBALL IS FREE AND MAY NOT BE SOLD.");

    menu_clearscores.push_back("SAVED SCORES CLEARED!");

}


Menu::~Menu(void)
{
}

void Menu::init()
{   
    video.sprite_layer->set_x_clip(false); // Stop clipping in wide-screen mode.
    video.sprite_layer->reset();
    video.clear_text_ram();
    ologo.enable(LOGO_Y);

    // Setup palette, road and colours for background
    oinitengine.road_seg_master = roms.rom0.read32(ROAD_SEG_TABLE + (0x3b << 2));
    opalette.setup_sky_palette();
    opalette.setup_ground_color();
	opalette.setup_road_centre();
	opalette.setup_road_stripes();
	opalette.setup_road_side();
	opalette.setup_road_colour();
    otiles.setup_palette_default();

    oroad.init();
    oroad.road_ctrl = ORoad::ROAD_BOTH_P0;
    oroad.horizon_set = 1;
    oroad.horizon_base = HORIZON_DEST + 0x100;;

    set_menu(&menu_main);
    refresh_menu();

    // Reset audio, so we can play tones
    osoundint.has_booted = true;
    osoundint.init();

    save_settings = false;
    frame = 0;
}

void Menu::tick()
{
    // Skip odd frames at 60fps
    frame++;

    tick_menu();

    // Do Text
    video.clear_text_ram();
    draw_menu_options();
    
    // Do Animations at 30 fps
    if (config.fps == 60 && (frame & 1) == 0)
    {
        if (oroad.horizon_base > HORIZON_DEST)
        {
            oroad.horizon_base -= 2;
            if (oroad.horizon_base < HORIZON_DEST)
                oroad.horizon_base = HORIZON_DEST;
        }

        ologo.tick();
        osprites.sprite_copy();

	    if (osprites.do_sprite_swap)
	    {
		    osprites.do_sprite_swap = false;
		    video.sprite_layer->swap();
		    osprites.copy_palette_data();
	    }
    }

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

#define SELECTED(string) boost::starts_with(OPTION, string)

void Menu::tick_menu()
{
    // Tick Controls
    if (input.has_pressed(Input::DOWN))
    {
        osoundint.queue_sound(sound::BEEP1);

        if (++cursor >= (int16_t) menu_selected->size())
            cursor = 0;
    }
    else if (input.has_pressed(Input::UP))
    {
        osoundint.queue_sound(sound::BEEP1);

        if (--cursor < 0)
            cursor = menu_selected->size() - 1;
    }
    else if (input.has_pressed(Input::BUTTON1))
    {
        osoundint.queue_sound(sound::BEEP1);

        // Get option that was selected
        const char* OPTION = menu_selected->at(cursor).c_str();

        if (menu_selected == &menu_main)
        {
            if (SELECTED(ENTRY_PLAYGAME))
            {
                if (save_settings) config.save("config.xml"); // Save settings
                cannonball::state = cannonball::STATE_INIT_GAME;
                osoundint.queue_clear();
            }
            else if (SELECTED(ENTRY_SETTINGS))
                set_menu(&menu_settings);
            else if (SELECTED(ENTRY_ABOUT))
                set_menu(&menu_about);
            else if (SELECTED(ENTRY_EXIT))
                cannonball::state = cannonball::STATE_QUIT;
        }
        else if (menu_selected == &menu_settings)
        {
            // We've entered the settings menu, so let's save settings to be safe.
            save_settings = true;

            if (SELECTED(ENTRY_VIDEO))
                set_menu(&menu_video);
            else if (SELECTED(ENTRY_SOUND))
                set_menu(&menu_sound);
            else if (SELECTED(ENTRY_CONTROLS))
                set_menu(&menu_controls);
            else if (SELECTED(ENTRY_ENGINE))
                set_menu(&menu_engine);
            else if (SELECTED(ENTRY_SCORES))
            {
                config.clear_scores();
                set_menu(&menu_clearscores);
            }
            else if (SELECTED(ENTRY_BACK))
                set_menu(&menu_main);
        }
        else if (menu_selected == &menu_about)
        {
            set_menu(&menu_main);
        }
        else if (menu_selected == &menu_video)
        {
            if (SELECTED(ENTRY_FPS))
            {
                if (++config.video.fps > 2)
                    config.video.fps = 0;

                config.set_fps(config.video.fps);
            }
            else if (SELECTED(ENTRY_BACK))
                set_menu(&menu_settings);
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
            else if (SELECTED(ENTRY_BACK))
                set_menu(&menu_settings);
        }
        else if (menu_selected == &menu_controls)
        {
            if (SELECTED(ENTRY_GEAR))
            {
                if (++config.controls.gear > 2)
                    config.controls.gear = 0;
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
            if (SELECTED(ENTRY_TIME))
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

            if (SELECTED(ENTRY_BACK))
                set_menu(&menu_settings);
        }
        else
            set_menu(&menu_main);

        refresh_menu();
    }
}

// Set Current Menu
void Menu::set_menu(std::vector<std::string> *menu)
{
    menu_selected = menu;
    cursor = 0;

    is_text_menu =  (menu == &menu_about || menu == &menu_clearscores);
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

        if (menu_selected == &menu_video)
        {
            if (SELECTED(ENTRY_FPS))
            {               
                if (config.video.fps == 0)      s = "30 FPS";
                else if (config.video.fps == 1) s = "ORIGINAL";
                else if (config.video.fps == 2) s = "60 FPS";
                set_menu_text(ENTRY_FPS, s);
            }
        }
        else if (menu_selected == &menu_sound)
        {
            if (SELECTED(ENTRY_MUTE))
                set_menu_text(ENTRY_MUTE, config.sound.enabled ? "ON" : "OFF");
            else if (SELECTED(ENTRY_ADVERTISE))
                set_menu_text(ENTRY_ADVERTISE, config.sound.advertise ? "ON" : "OFF");
        }
        else if (menu_selected == &menu_controls)
        {
            if (SELECTED(ENTRY_GEAR))
            {
                if (config.controls.gear == 0)      s = "MANUAL NORMAL";
                else if (config.controls.gear == 1) s = "MANUAL CABINET";
                else if (config.controls.gear == 2) s = "AUTOMATIC";
                set_menu_text(ENTRY_GEAR, s);
            }
            else if (SELECTED(ENTRY_DSTEER))
                set_menu_text(ENTRY_DSTEER, config.to_string(config.controls.steer_speed));
            else if (SELECTED(ENTRY_DPEDAL))
                set_menu_text(ENTRY_DPEDAL, config.to_string(config.controls.pedal_speed));
        }
        else if (menu_selected == &menu_engine)
        {
            if (SELECTED(ENTRY_TIME))
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