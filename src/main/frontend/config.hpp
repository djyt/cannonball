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

struct ttrial_settings_t
{
    int laps;
    int traffic;
    uint16_t best_times[15];
};

struct menu_settings_t
{
    int enabled;
    int road_scroll_speed;
};

struct video_settings_t
{
    const static int MODE_WINDOW  = 0;
    const static int MODE_FULL    = 1;
    const static int MODE_STRETCH = 2;

    int mode;
    int scale;
    int scanlines;
    int widescreen;
    int fps;
    int hires;
};

struct sound_settings_t
{
    int enabled;
    int advertise;
    custom_music_t custom_music[4];
};

struct controls_settings_t
{
    const static int GEAR_BUTTON = 0;
    const static int GEAR_PRESS  = 1; // For cabinets
    const static int GEAR_AUTO   = 2;

    int gear;
    int steer_speed;   // Steering Digital Speed
    int pedal_speed;   // Pedal Digital Speed
    int padconfig[6];  // Joypad Button Config
    int keyconfig[10]; // Keyboard Button Config
    int analog;        // Use analog controls
    int axis[3];       // Analog Axis
    int wheel[2];      // Wheel Settings

    int haptic;        // Force Feedback Enabled
    int max_force;
    int min_force;
    int force_duration;
};

struct engine_settings_t
{
    int dip_time;
    int dip_traffic;
    bool freeze_timer;
    bool disable_traffic;
    int jap;
    int prototype;
    int randomgen;
    int level_objects;
    bool fix_bugs;
};

class Config
{
public:
    menu_settings_t     menu;
    video_settings_t    video;
    sound_settings_t    sound;
    controls_settings_t controls;
    engine_settings_t   engine;
    ttrial_settings_t   ttrial;

    // Internal screen width and height
    uint16_t s16_width, s16_height;

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
    std::string to_string(int i);
    std::string to_string(char c);
   
private:
    // Conversions
    template<class T> std::string to_hex_string(T i);
    uint32_t from_hex_string(std::string s);
};

extern Config config;
