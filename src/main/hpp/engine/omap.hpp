#pragma once

#include "outrun.hpp"

class OMap
{
public:

    // Load Sprites Needed for Course Map
    bool init_sprites;

    OMap(void);
    ~OMap(void);

    void tick();

    void draw_horiz_end(oentry*);
    void draw_vert_bottom(oentry*);
    void draw_vert_top(oentry*);

    void draw_course_map();

private:
    // Total sprite pieces that comprise course map. 3c
    const static uint8_t MAP_PIECES = 0x3C;

    uint8_t map_state;

    enum
    {
        MAP_INIT  = 0,
        // Do Route [Note map is displayed from this point on]
        MAP_ROUTE = 0x4,
        // Do Final Segment Of Route [Car still moving]
        MAP_ROUTE_FINAL = 0x08,
        // Route Concluded
        MAP_ROUTE_DONE = 0x0C,
        // Init Delay Counter For Map Display
        MAP_INIT_DELAY = 0x10,
        // Display Map
        MAP_DISPLAY = 0x14,
        // Clear Course Map
        MAP_CLEAR = 0x18,
    };

    // Direction to move on mini-map

    // Bit 0 = 
    // 0 = Up (Left Route)
    // 1 = Down (Right Route)
    uint8_t map_route;

    // Minimap Position (Per Segment Basis)
    int16_t map_pos;

    // Minimap Position (Final Segment)
    int16_t map_pos_final;

    // Map Delay Counter
    int16_t map_delay;

    // Stage counter for course map screen. [Increments]
    int16_t map_stage1;

    // Stage counter for course map screen. [Decrements]
    // Loaded with stage, then counts down as course map logic runs.
    int16_t map_stage2;

    // Minicar Movement Enabled (set = enabled)
    uint8_t minicar_enable;

    void init_course_map();
    void draw_piece(oentry*, uint32_t);
    void do_route_final();
    void end_route();
    void init_map_delay();
    void map_display();
    void move_mini_car(oentry*);
};

extern OMap omap;
