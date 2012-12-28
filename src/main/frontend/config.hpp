/***************************************************************************
    XML Configuration File Handling.

    Load Settings.
    Load & Save Hi-Scores.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include <set>

#include "stdint.hpp"

struct video_settings_t
{
    const static int MODE_WINDOW = 0;
    const static int FULLSCREEN = 1;

    int widescreen;
    int mode;
    int scale;
    int stretch;
    int fps;
};

struct sound_settings_t
{
    int enabled;
    int advertise;
};

struct controls_settings_t
{
    int gear;
    int steer_speed; // Steering Digital Speed
    int pedal_speed; // Pedal Digital Speed
};

struct engine_settings_t
{
    const static int GEAR_PRESS = 1; // For cabinets
    const static int GEAR_AUTO  = 2;

    int dip_time;
    int dip_traffic;
    bool freeze_timer;
    bool disable_traffic;
    int level_objects;
};

class Config
{
public:
    video_settings_t video;
    sound_settings_t sound;
    controls_settings_t controls;
    engine_settings_t engine;

    // Internal screen width
    uint16_t s16_width;

    // Internal screen x offset
    uint16_t s16_x_off;

    // Use internal menu system?
    int use_menu;

    // 30 or 60 fps
    int fps;

    // Original game ticks sprites at 30fps but background scroll at 60fps
    int tick_fps;

    Config(void);
    ~Config(void);

    void init();
    void load(const std::string &filename);
    void save(const std::string &filename);
    void load_scores();
    void save_scores();
    void clear_scores();
    void set_fps(int fps);

    template<class T> std::string to_string(T i);
   
private:
    // Conversions
    template<class T> std::string to_hex_string(T i);
    uint32_t from_hex_string(std::string s);
};

extern Config config;
