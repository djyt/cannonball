/***************************************************************************
    Process Inputs.
    
    - Read & Process inputs and controls.
    - Note, this class does not contain platform specific code.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <iostream>

#include "engine/ocrash.hpp"
#include "engine/oinputs.hpp"
#include "engine/ostats.hpp"

#include "cannonboard/interface.hpp"

OInputs oinputs;

OInputs::OInputs(void)
{
}

OInputs::~OInputs(void)
{
}

void OInputs::init()
{
    input_steering  = STEERING_CENTRE;
    steering_old    = STEERING_CENTRE;
    steering_adjust = 0;
    acc_adjust      = 0;
    brake_adjust    = 0;
    steering_change = 0;

    steering_inc = config.controls.steer_speed;
    acc_inc      = config.controls.pedal_speed * 4;
    brake_inc    = config.controls.pedal_speed * 4;

    input_acc   = 0;
    input_brake = 0;
    gear        = false;
    crash_input = 0;
    delay1      = 0;
    delay2      = 0;
    delay3      = 0;
    coin1       = false;
    coin2       = false;
}

void OInputs::tick(Packet* packet)
{
    // CannonBoard Input
    if (packet != NULL)
    {
        input_steering = packet->ai2;
        input_acc      = packet->ai0;
        input_brake    = packet->ai3;
            
        if (config.controls.gear != config.controls.GEAR_AUTO)
            gear       = (packet->di1 & 0x10) == 0;

        // Coin Chutes
        coin1 = (packet->di1 & 0x40) != 0;
        coin2 = (packet->di1 & 0x80) != 0;

        // Service
        input.keys[Input::COIN]  = (packet->di1 & 0x04) != 0;
        // Start
        input.keys[Input::START] = (packet->di1 & 0x08) != 0;
    }

    // Standard PC Keyboard/Joypad/Wheel Input
    else
    {
        // Digital Controls: Simulate Analog
        if (!input.analog || !input.gamepad)
        {
            digital_steering();
            digital_pedals();
        }
        // Analog Controls
        else
        {
            input_steering = input.a_wheel;

            // Analog Pedals
            if (input.analog == 1)
            {
                input_acc      = input.a_accel;
                input_brake    = input.a_brake;
            }
            // Digital Pedals
            else
            {
                digital_pedals();
            }
        }
    }
}
// DIGITAL CONTROLS: Digital Simulation of analog steering
void OInputs::digital_steering()
{
    // ------------------------------------------------------------------------
    // STEERING
    // ------------------------------------------------------------------------
    if (input.is_pressed(Input::LEFT))
    {
        // Recentre wheel immediately if facing other way
        if (input_steering > STEERING_CENTRE) input_steering = STEERING_CENTRE;

        input_steering -= steering_inc;
        if (input_steering < STEERING_MIN) input_steering = STEERING_MIN;
    }
    else if (input.is_pressed(Input::RIGHT))
    {
        // Recentre wheel immediately if facing other way
        if (input_steering < STEERING_CENTRE) input_steering = STEERING_CENTRE;

        input_steering += steering_inc;
        if (input_steering > STEERING_MAX) input_steering = STEERING_MAX;
    }
    // Return steering to centre if nothing pressed
    else
    {
        if (input_steering < STEERING_CENTRE)
        {
            input_steering += steering_inc;
            if (input_steering > STEERING_CENTRE)
                input_steering = STEERING_CENTRE;
        }
        else if (input_steering > STEERING_CENTRE)
        {
            input_steering -= steering_inc;
            if (input_steering < STEERING_CENTRE)
                input_steering = STEERING_CENTRE;
        }
    }
}

// DIGITAL CONTROLS: Digital Simulation of analog pedals
void OInputs::digital_pedals()
{
    // ------------------------------------------------------------------------
    // ACCELERATION
    // ------------------------------------------------------------------------

    if (input.is_pressed(Input::ACCEL))
    {
        input_acc += acc_inc;
        if (input_acc > 0xFF) input_acc = 0xFF;
    }
    else
    {
        input_acc -= acc_inc;
        if (input_acc < 0) input_acc = 0;
    }

    // ------------------------------------------------------------------------
    // BRAKE
    // ------------------------------------------------------------------------

    if (input.is_pressed(Input::BRAKE))
    {
        input_brake += brake_inc;
        if (input_brake > 0xFF) input_brake = 0xFF;
    }
    else
    {
        input_brake -= brake_inc;
        if (input_brake < 0) input_brake = 0;
    }
}

void OInputs::do_gear()
{
    if (config.cannonboard.enabled)
        return;

    // ------------------------------------------------------------------------
    // GEAR SHIFT
    // ------------------------------------------------------------------------

    // Automatic Gears: Don't do anything
    if (config.controls.gear == config.controls.GEAR_AUTO)
        return;

    else
    {
        // Manual: Cabinet Shifter
        if (config.controls.gear == config.controls.GEAR_PRESS)
            gear = !input.is_pressed(Input::GEAR1);

        // Manual: Two Separate Buttons for gears
        else if (config.controls.gear == config.controls.GEAR_SEPARATE)
        {
            if (input.has_pressed(Input::GEAR1))
                gear = false;
            else if (input.has_pressed(Input::GEAR2))
                gear = true;
        }

        // Manual: Keyboard/Digital Button
        else
        {
            if (input.has_pressed(Input::GEAR1))
                gear = !gear;
        }
    }
}

// Adjust Analogue Inputs
//
// Read, Adjust & Write Analogue Inputs
// In the original, these values are set during a H-Blank routine
//
// Source: 74E2

void OInputs::adjust_inputs()
{
    // Cap Steering Value
    if (input_steering < STEERING_MIN) input_steering = STEERING_MIN;
    else if (input_steering > STEERING_MAX) input_steering = STEERING_MAX;

    if (crash_input)
    {
        crash_input--;
        int16_t d0 = ((input_steering - 0x80) * 0x100) / 0x70;
        if (d0 > 0x7F) d0 = 0x7F;
        else if (d0 < -0x7F) d0 = -0x7F;
        steering_adjust = ocrash.crash_counter ? 0 : d0;
    }
    else
    {
        // no_crash1:
        int16_t d0 = input_steering - steering_old;
        steering_old = input_steering;
        steering_change += d0;
        d0 = steering_change < 0 ? -steering_change : steering_change;

        // Note the below line "if (d0 > 2)" causes a bug in the original game
        // whereby if you hold the wheel to the left whilst stationary, then accelerate the car will veer left even
        // when the wheel has been centered
        if (config.engine.fix_bugs || d0 > 2)
        {
            steering_change = 0;
            // Convert input steering value to internal value
            d0 = ((input_steering - 0x80) * 0x100) / 0x70;
            if (d0 > 0x7F) d0 = 0x7F;
            else if (d0 < -0x7F) d0 = -0x7F;
            steering_adjust = ocrash.crash_counter ? 0 : d0;
        }
    }

    // Cap & Adjust Acceleration Value
    int16_t acc = input_acc;
    if (acc > PEDAL_MAX) acc = PEDAL_MAX;
    else if (acc < PEDAL_MIN) acc = PEDAL_MIN;
    acc_adjust = ((acc - 0x30) * 0x100) / 0x61;

    // Cap & Adjust Brake Value
    int16_t brake = input_brake;
    if (brake > PEDAL_MAX) brake = PEDAL_MAX;
    else if (brake < PEDAL_MIN) brake = PEDAL_MIN;
    brake_adjust = ((brake - 0x30) * 0x100) / 0x61;
}

// Simplified version of do credits routine. 
// I have not ported the coin chute handling code, or dip switch routines.
//
// Returns: 0 (No Coin Inserted)
//          1 (Coin Chute 1 Used)
//          2 (Coin Chute 2 Used)
//          3 (Key Pressed / Service Button)
//
// Source: 0x6DE0
uint8_t OInputs::do_credits()
{
    if (input.has_pressed(Input::COIN))
    {
        input.keys[Input::COIN] = false; // immediately clear due to this routine being in vertical interrupt
        if (!config.engine.freeplay && ostats.credits < 9)
        {
            ostats.credits++;
            // todo: Increment credits total for bookkeeping
            osoundint.queue_sound(sound::COIN_IN);
        }
        return 3;
    }
    else if (coin1)
    {
        coin1 = false;
        if (!config.engine.freeplay && ostats.credits < 9)
        {
            ostats.credits++;
            osoundint.queue_sound(sound::COIN_IN);
        }
        return 1;
    }
    else if (coin2)
    {
        coin2 = false;
        if (!config.engine.freeplay && ostats.credits < 9)
        {
            ostats.credits++;
            osoundint.queue_sound(sound::COIN_IN);
        }
        return 2;
    }
    return 0;
}

// ------------------------------------------------------------------------------------------------
// Menu Selection Controls
// ------------------------------------------------------------------------------------------------

bool OInputs::is_analog_l()
{
    if (input_steering < STEERING_CENTRE - 0x10)
    {
        if (--delay1 < 0)
        {
            delay1 = DELAY_RESET;
            return true;
        }
    }
    else
        delay1 = DELAY_RESET;
    return false;
}

bool OInputs::is_analog_r()
{
    if (input_steering > STEERING_CENTRE + 0x10)
    {
        if (--delay2 < 0)
        {
            delay2 = DELAY_RESET;
            return true;
        }
    }
    else
        delay2 = DELAY_RESET;
    return false;
}

bool OInputs::is_analog_select()
{
    if (input_acc > 0x90)
    {
        if (--delay3 < 0)
        {
            delay3 = DELAY_RESET;
            return true;
        }
    }
    else
        delay3 = DELAY_RESET;
    return false;
}