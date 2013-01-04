/***************************************************************************
    XML Configuration File Handling.

    Load Settings.
    Load & Save Hi-Scores.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include <set>
#include <string>
#include "stdint.hpp"

struct custom_music_t
{
    int enabled;
    std::string title;
    std::string filename;
};

struct menu_settings_t
{
    int enabled;
    int road_scroll_speed;
};

struct video_settings_t
{
    const static int MODE_WINDOW = 0;
    const static int FULLSCREEN = 1;

    int mode;
    int scale;
    int stretch;
    int scanlines;
    int widescreen;
    int fps;
};

struct sound_settings_t
{
    int enabled;
    int advertise;
    custom_music_t custom_music[4];
};

struct controls_settings_t
{
    int gear;
    int steer_speed;   // Steering Digital Speed
    int pedal_speed;   // Pedal Digital Speed
    int padconfig[6];  // Joypad Button Config
    int keyconfig[10]; // Keyboard Button Config
};

struct engine_settings_t
{
    const static int GEAR_PRESS = 1; // For cabinets
    const static int GEAR_AUTO  = 2;

    int dip_time;
    int dip_traffic;
    bool freeze_timer;
    bool disable_traffic;
    int jap;
    int randomgen;
    int level_objects;
};

class Config
{
public:
    menu_settings_t menu;
    video_settings_t video;
    sound_settings_t sound;
    controls_settings_t controls;
    engine_settings_t engine;

    // Internal screen width
    uint16_t s16_width;

    // Internal screen x offset
    uint16_t s16_x_off;

    // 30 or 60 fps
    int fps;

    // Original game ticks sprites at 30fps but background scroll at 60fps
    int tick_fps;
    
    Config(void);
    ~Config(void);

    void init();
    void load(const std::string &filename);
    bool save(const std::string &filename);
    void load_scores();
    void save_scores();
    bool clear_scores();
    void set_fps(int fps);

    template<class T> std::string to_string(T i);
   
private:
    // Conversions
    template<class T> std::string to_hex_string(T i);
    uint32_t from_hex_string(std::string s);
};

extern Config config;
