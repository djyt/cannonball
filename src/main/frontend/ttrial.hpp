#pragma once

#include "stdint.hpp"

class TTrial
{
public:
    // Maximum number of laps to allow the player to race
    static const uint8_t MAX_LAPS = 5;

    // Maximum number of cars to spawn
    static const uint8_t MAX_TRAFFIC = 8;

    // Counter value that represents 1m 15s 0ms
    static const uint16_t COUNTER_1M_15 = 0x12C0;

    uint8_t state;
    int8_t level_selected;

    enum
    {
        INIT_COURSEMAP,
        TICK_COURSEMAP,
        TICK_GAME_ENGINE,
    };

    TTrial(void);
    ~TTrial(void);

    void init();
    bool tick();

private:
    // Best lap times for all 15 tracks.
    uint8_t best_times[15][3];

};