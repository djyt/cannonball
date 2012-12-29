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

void Input::init(int* key_config, int* pad_config)
{
    this->key_config = key_config;
    this->pad_config = pad_config;

    gamepad = SDL_NumJoysticks() >= 1;

    if (gamepad)
    {
        stick = SDL_JoystickOpen(0);
    }
}

void Input::stop()
{
    if (gamepad && stick != NULL)
        SDL_JoystickClose(stick);
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
    memcpy(&keys_old, &keys, sizeof(keys));
}

void Input::handle_key_down(SDL_keysym* keysym)
{
    key_press = keysym->sym;
    handle_key(key_press, true);
}

void Input::handle_key_up(SDL_keysym* keysym)
{
    handle_key(keysym->sym, false);
}

void Input::handle_key(const int key, const bool is_pressed)
{
    // Redefinable Key Input
    if (key == key_config[0])
        keys[UP] = is_pressed;

    else if (key == key_config[1])
        keys[DOWN] = is_pressed;

    else if (key == key_config[2])
        keys[LEFT] = is_pressed;

    else if (key == key_config[3])
        keys[RIGHT] = is_pressed;

    else if (key == key_config[4])
        keys[ACCEL] = is_pressed;

    else if (key == key_config[5])
        keys[BRAKE] = is_pressed;

    else if (key == key_config[6])
        keys[GEAR] = is_pressed;

    else if (key == key_config[7])
        keys[START] = is_pressed;

    else if (key == key_config[8])
        keys[COIN] = is_pressed;

    else if (key == key_config[9])
        keys[MENU] = is_pressed;

    // Function keys are not redefinable
    switch (key)
    {
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
            keys[MENU] = is_pressed;
            break;
    }
}

void Input::handle_joy_axis(SDL_JoyAxisEvent* evt)
{
    int16_t value = evt->value;

    // X-Axis
    if (evt->axis == 0)
    {
        // Neural
        if ( (value > -3200 ) && (value < 3200 ) )
        {
            keys[LEFT]  = false;
            keys[RIGHT] = false;
        }
        else if (value < 0)
        {
            keys[LEFT] = true;
        }
        else if (value > 0)
        {
            keys[RIGHT] = true;
        }
    }
    // Y-Axis
    else if (evt->axis == 1)
    {
        // Neural
        if ( (value > -3200 ) && (value < 3200 ) )
        {
            keys[UP]  = false;
            keys[DOWN] = false;
        }
        else if (value < 0)
        {
            keys[UP] = true;
        }
        else if (value > 0)
        {
            keys[DOWN] = true;
        }
    }
}

void Input::handle_joy_down(SDL_JoyButtonEvent* evt)
{
    // Latch joystick button presses for redefines
    joy_button = evt->button;
    handle_joy(evt->button, true);
}

void Input::handle_joy_up(SDL_JoyButtonEvent* evt)
{
    handle_joy(evt->button, false);
}

void Input::handle_joy(const uint8_t button, const bool is_pressed)
{
    if (button == pad_config[0])
        keys[ACCEL] = is_pressed;

    if (button == pad_config[1])
        keys[BRAKE] = is_pressed;

    if (button == pad_config[2])
        keys[GEAR] = is_pressed;

    if (button == pad_config[3])
        keys[START] = is_pressed;

    if (button == pad_config[4])
        keys[COIN] = is_pressed;

    if (button == pad_config[5])
        keys[MENU] = is_pressed;
}