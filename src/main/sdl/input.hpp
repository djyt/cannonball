/***************************************************************************
    SDL Based Input Handling.

    Populates keys array with user input.
    If porting to a non-SDL platform, you would need to replace this class.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include <SDL.h>

class Input
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

        START = 7,
        COIN  = 8,
        
        // Debug 
        PAUSE = 9,
        STEP  = 10,
        TIMER = 11,
        END_SEQ = 12,
    };

    bool keys[13];
    bool keys_old[13];

    Input(void);
    ~Input(void);

    void handle_key_up(SDL_keysym*);
    void handle_key_down(SDL_keysym*);
    void frame_done();
    bool is_pressed(presses p);
    bool has_pressed(presses p);

private:
    void handle_key(SDL_keysym*, bool);
};

extern Input input;