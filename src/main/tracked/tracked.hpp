#pragma once

#include "stdint.hpp"

class Tracked
{
public:
    Tracked(void);
    ~Tracked(void);

    void init();
    void tick();

private:

    enum states
    {
        INIT = 0,
        TICK = 1,
    };

    uint8_t state;

    void tick_track();
    void controls();
    void display_sprite_info();
    std::string dec_to_bin(int nValue, bool bReverse = false);
};