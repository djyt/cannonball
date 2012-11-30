/***************************************************************************
    SDL Based Input Handling.

    Populates keys array with user input.
    If porting to a non-SDL platform, you would need to replace this class.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "sdl/input.hpp"

Input input;

Input::Input(void)
{
}

Input::~Input(void)
{
}

// Detect whether a key press change has occurred
bool Input::has_pressed(presses p)
{
    return keys[p] && !keys_old[p];
}

// Detect whether key is still pressed
bool Input::is_pressed(presses p)
{
    return keys[p];
}

// Denote that a frame has been done by copying key presses into previous array
void Input::frame_done()
{
    memcpy(keys_old, keys, sizeof(keys));
}

void Input::handle_key_down(SDL_keysym* keysym)
{
    handle_key(keysym, true);
    return;
}

void Input::handle_key_up(SDL_keysym* keysym)
{
    handle_key(keysym, false);
    return;
}

void Input::handle_key(SDL_keysym* keysym, bool is_pressed)
{
    switch(keysym->sym)
    {
        case SDLK_UP:
            keys[UP] = is_pressed;
            break;

        case SDLK_DOWN:
            keys[DOWN] = is_pressed;
            break;

        case SDLK_LEFT:
            keys[LEFT] = is_pressed;
            break;

        case SDLK_RIGHT:
            keys[RIGHT] = is_pressed;
            break;

        case SDLK_z:
            keys[BUTTON1] = is_pressed;
            break;

        case SDLK_x:
            keys[BUTTON2] = is_pressed;
            break;

        case SDLK_SPACE:
            keys[BUTTON3] = is_pressed;
            break;

        case SDLK_1:
            keys[START] = is_pressed;
            break;

        case SDLK_5:
            keys[COIN] = is_pressed;
            break;

        case SDLK_F1:
            keys[PAUSE] = is_pressed;
            break;

        case SDLK_F2:
            keys[STEP] = is_pressed;
            break;

        case SDLK_F3:
            keys[TIMER] = is_pressed;
            break;

        case SDLK_F5:
            keys[END_SEQ] = is_pressed;
            break;
        
        default:
            break;
    }
}