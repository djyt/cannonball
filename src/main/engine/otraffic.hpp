/***************************************************************************
    Traffic Routines.

    - Traffic spawning.
    - Traffic logic, lane changing & movement.
    - Collisions.
    - Traffic panning and volume control to pass to sound program.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.hpp"

class OTraffic
{
public:
    // AI: Set to denote enemy traffic is close to car.
    uint8_t ai_traffic;

    // Set to denote we should go to LHS of road on bonus
    uint8_t bonus_lhs;

    // Logic for traffic based on road split
    int8_t traffic_split;

    // Denotes Collision With Other Traffic
    //
    // 0 = No Collision
    // 1 = Init Collision Sequence
    // 2 = Collision Sequence In Progress
    uint16_t collision_traffic;

    uint16_t collision_mask;

    OTraffic(void);
    ~OTraffic(void);
    void init();
    void init_stage1_traffic();
    void tick();
    void disable_traffic();
    void set_max_traffic();
    void traffic_logic();
    void traffic_sound();

private:
	// -------------------------------------------------------------------------
	// Function Holders
	// -------------------------------------------------------------------------
    enum
    {
        TRAFFIC_INIT = 0x10,       // Initalize Traffic Object
        TRAFFIC_ENTRY = 0x11,      // First 0x80 Positions Of Road
        TRAFFIC_TICK = 0x12        // Tick Normally
    };

    // Onscreen traffic objects
    oentry* traffic_adr[8];

    // Maximum number of on-screen enemies
    uint8_t max_traffic;

    // Total speed of all traffic combined
    int16_t traffic_speed_total;

    // Average speed of traffic. Used to control wheel frames of traffic sprites.
    int16_t traffic_speed_avg;

    // Traffic Palette Cycle. This alternates between 0 and 1.
    // Essentially changes the palette, so that the wheels on the traffic appear to be in motion.
    uint8_t traffic_pal_cycle;

    // Number of traffic spawned
    int16_t traffic_count;

    // +1E [Word] Spawn Tick Counter. Used as a somewhat unrandom way of spawning cars.
    int16_t spawn_counter;

    // +20 [Word] Left Hand / Right Hand Spawn Control. Controls where next car should spawn.
    int16_t spawn_location;

    // +22 [Word] Wheel Animation Reset Value
    int16_t wheel_reset;

    // +24 [Word] Wheel Animation Counter
    int16_t wheel_counter;

    void spawn_car(oentry* sprite);
    void spawn_traffic();
    void tick_spawned_sprite(oentry* sprite);
    void move_spawned_sprite(oentry* sprite);
    void update_props(oentry* sprite);
    void set_zoom_lookup(oentry* sprite);
    void calculate_avg_speed(uint16_t);
    void check_collision(oentry* sprite);
};

extern OTraffic otraffic;