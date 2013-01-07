#pragma once

#include "stdint.hpp"

class TTrial
{
public:
    uint8_t state;

    enum
    {
        INIT_COURSEMAP,
        TICK_COURSEMAP,
        TICK_GAME_ENGINE,
    };

    TTrial(void);
    ~TTrial(void);

    void init();
    void tick();

private:

};