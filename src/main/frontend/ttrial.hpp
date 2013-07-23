/***************************************************************************
    Time Trial Mode Front End.

    This file is part of Cannonball. 
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "../stdint.hpp"

class TTrial
{
public:
    // Maximum number of laps to allow the player to race
    static const uint8_t MAX_LAPS = 5;

    // Maximum number of cars to spawn
    static const uint8_t MAX_TRAFFIC = 8;

    enum
    {
        BACK_TO_MENU = -1,
        CONTINUE = 0,
        INIT_GAME = 1,
    };

    uint8_t state;
    int8_t level_selected;

    enum
    {
        INIT_COURSEMAP,
        TICK_COURSEMAP,
        TICK_GAME_ENGINE,
    };

    TTrial(uint16_t* best_times);
    ~TTrial(void);

    void init();
    int  tick();
    void update_best_time();

private:
    // Best lap times for all 15 tracks.
    uint16_t* best_times;

    // Counter converted to actual laptime
    uint8_t best_converted[3];
};