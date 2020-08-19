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

    // JJP - Blargg CRT filtering constants
    const static int BLARGG_DISABLE = 0;
    const static int BLARGG_COMPOSITE = 1;
    const static int BLARGG_SVIDEO = 2;
    const static int BLARGG_RGB = 3;
    const static int BLARGG_MONO = 4;

    int fps;
    int fps_count;
    int mode;              // Full screen or Windowed
    int scale;             // now integrated into mode in UI (Fullscreen/Window1x/Windows2x/Window3x/Window4x)
    int widescreen;
    int hires;
    int scanlines;         // Provides
    int mask;              // various CRT effects
    int mask_strength;     // Blend strength of selected effect
    int vignette;          // progressively dims screen away from centre
    // JJP - Blargg related filtering settings
    int blargg;            // Blargg mode - per above constants
    int blarggthreads;     // Filter is intensive, this spreads load across cores
    int saturation;
    int contrast;
    int brightness;
    int sharpness;
    int gamma;
    // No UI for the following
    int filtering;         // OpenGL blend mode, 0 or 1 on Linux
    int flicker;           // Slightly dims alternate frames
};

struct sound_settings_t
{
    int enabled;
    int advertise;
    int preview;
    int fix_samples;
    custom_music_t custom_music[4];
};

struct controls_settings_t
{
    const static int GEAR_BUTTON   = 0;
    const static int GEAR_PRESS    = 1; // For cabinets
    const static int GEAR_SEPARATE = 2; // Separate button presses
    const static int GEAR_AUTO     = 3;

    int gear;
    int steer_speed;   // Steering Digital Speed
    int pedal_speed;   // Pedal Digital Speed
    int padconfig[8];  // Joypad Button Config
    int keyconfig[12]; // Keyboard Button Config
    int pad_id;        // Use the N'th joystick on the system.
    int analog;        // Use analog controls
    int axis[3];       // Analog Axis
    int asettings[3];  // Analog Settings

    int haptic;        // Force Feedback Enabled
    int max_force;
    int min_force;
    int force_duration;
};

struct cannonboard_settings_t
{
    const static int CABINET_MOVING  = 0;
    const static int CABINET_UPRIGHT = 1;
    const static int CABINET_MINI    = 2;

    int enabled;      // CannonBall used in conjunction with CannonBoard in arcade cabinet
    std::string port; // Port Name
    int baud;         // Baud Rate
    int debug;        // Display Debug Information
    int cabinet;      // Cabinet Type
};

struct engine_settings_t
{
    int dip_time;
    int dip_traffic;
    bool freeplay;
    bool freeze_timer;
    bool disable_traffic;
    int jap;
    int prototype;
    int randomgen;
    int level_objects;
    bool fix_bugs;
    bool fix_bugs_backup;
    bool fix_timer;
    bool layout_debug;
    int new_attract;
};

class Config
{
public:
    menu_settings_t        menu;
    video_settings_t       video;
    sound_settings_t       sound;
    controls_settings_t    controls;
    engine_settings_t      engine;
    ttrial_settings_t      ttrial;
    cannonboard_settings_t cannonboard;

    // Internal screen width and height
    uint16_t s16_width, s16_height;

    // Internal screen x offset
    uint16_t s16_x_off;

    // 30 or 60 fps
    int fps;

    // Original game ticks sprites at 30fps but background scroll at 60fps
    int tick_fps;

    // Continuous Mode: Traffic Setting
    int cont_traffic;
    
    Config(void);
    ~Config(void);

    void init();
    void load(const std::string &filename);
    bool save(const std::string &filename);
    void load_scores(const std::string &filename);
    void save_scores(const std::string &filename);
    void load_tiletrial_scores();
    void save_tiletrial_scores();
    bool clear_scores();
    void set_fps(int fps);
   
private:
};

extern Config config;
