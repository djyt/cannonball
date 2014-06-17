/***************************************************************************
    Process Inputs.
    
    - Read & Process inputs and controls.
    - Note, this class does not contain platform specific code.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "sdl/input.hpp"
#include "outrun.hpp"

struct Packet;

class OInputs
{
public:

    const static uint8_t BRAKE_THRESHOLD1 = 0x80;
    const static uint8_t BRAKE_THRESHOLD2 = 0xA0;
    const static uint8_t BRAKE_THRESHOLD3 = 0xC0;
    const static uint8_t BRAKE_THRESHOLD4 = 0xE0;

    int8_t crash_input;

    // Acceleration Input
    int16_t input_acc;

    // Steering Input
    int16_t input_steering;

    // Processed / Adjusted Values
    int16_t steering_adjust;
    int16_t acc_adjust;
    int16_t brake_adjust;
    
    // True = High Gear. False = Low Gear.
    bool gear;

    OInputs(void);
    ~OInputs(void);

    void init();
    void tick(Packet* packet);
    void adjust_inputs();
    void do_gear();
    uint8_t do_credits();

    bool is_analog_l();
    bool is_analog_r();
    bool is_analog_select();

private:
    // ------------------------------------------------------------------------
    // Variables for port
    // ------------------------------------------------------------------------

    // Amount to adjust steering per tick. (0x3 is a good test value)
    uint8_t steering_inc;

    // Amount to adjust acceleration per tick. (0x10 is a good test value)
    uint8_t acc_inc;

    // Amount to adjust brake per tick. (0x10 is a good test value)
    uint8_t brake_inc;

    static const int DELAY_RESET = 60;
    int delay1, delay2, delay3;

    // Coin Inputs (Only used by CannonBoard)
    bool coin1, coin2;

    // ------------------------------------------------------------------------
    // Variables from original code
    // ------------------------------------------------------------------------

    const static uint8_t STEERING_MIN = 0x48;
    const static uint8_t STEERING_MAX = 0xB8;
    const static uint8_t STEERING_CENTRE = 0x80;
    
    // Current steering value
    int16_t steering_old;
    int16_t steering_change;

    const static uint8_t PEDAL_MIN = 0x30;
    const static uint8_t PEDAL_MAX = 0x90;

    // Brake Input
    int16_t input_brake;

    void digital_steering();
    void digital_pedals();
};

extern OInputs oinputs;