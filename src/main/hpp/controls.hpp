#pragma once

#include <SDL.h>

//#include "stdint.hpp"
#include "outrun.hpp"

class Controls
{
public:
    enum presses
    {
        LEFT = 0,
        RIGHT = 1,
        UP = 2,
        DOWN = 3,
        BUTTON1 = 4,
        BUTTON2 = 5,
        BUTTON3 = 6,
    };

    bool keys[10];
    bool keys_old[10];

    Controls(void);
    ~Controls(void);

    void handle_key_up(SDL_keysym*);
    void handle_key_down(SDL_keysym*);
    void frame_done();
    bool is_pressed(presses p);
    bool has_pressed(presses p);
};

extern Controls controls;

