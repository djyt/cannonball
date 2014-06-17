/***************************************************************************
    Process Outputs.

    - Cabinet Vibration & Hydraulic Movement
    - Brake & Start Lamps
    - Coin Chute Outputs
    
    The Deluxe Motor code is also used by the force-feedback haptic system.

    One thing to note is that this code was originally intended to drive
    a moving hydraulic cabinet, not to be mapped to a haptic device.

    Therefore, it's not perfect when used in this way, but the results
    aren't bad :)
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <cstdlib> // abs

#include "utils.hpp"

#include "engine/outrun.hpp"
#include "engine/ocrash.hpp"
#include "engine/oferrari.hpp"
#include "engine/ohud.hpp"
#include "engine/oinputs.hpp"
#include "engine/ooutputs.hpp"
#include "directx/ffeedback.hpp"

OOutputs::OOutputs(void)
{
    chute1.output_bit  = D_COIN1_SUCC;
    chute2.output_bit  = D_COIN2_SUCC;

    col1               = 0;
    col2               = 0;
    limit_left         = 0;
    limit_right        = 0;
    motor_enabled      = true;
}

OOutputs::~OOutputs(void)
{
}


// Initalize Moving Cabinet Motor
// Source: 0xECE8
void OOutputs::init()
{
    motor_state        = STATE_INIT;
    hw_motor_control   = MOTOR_OFF;
    dig_out            = 0;
    motor_control      = 0;
    motor_movement     = 0;
    is_centered        = false;
    motor_change_latch = 0;
    speed              = 0;
    curve              = 0;
    counter            = 0;
    vibrate_counter    = 0;
    was_small_change   = false;
    movement_adjust1   = 0;
    movement_adjust2   = 0;
    movement_adjust3   = 0;
    chute1.counter[0]  = 0;
    chute1.counter[1]  = 0;
    chute1.counter[2]  = 0;
    chute2.counter[0]  = 0;
    chute2.counter[1]  = 0;
    chute2.counter[2]  = 0;
}

void OOutputs::tick(int MODE, int16_t input_motor, int16_t cabinet_type)
{
    switch (MODE)
    {
        // Force Feedback Steering Wheels
        case MODE_FFEEDBACK:
            do_motors(MODE, input_motor);   // Use X-Position of wheel instead of motor position
            motor_output(hw_motor_control); // Force Feedback Handling
            break;

        // CannonBoard: Real Cabinet
        case MODE_CABINET:
            if (cabinet_type == config.cannonboard.CABINET_MOVING)
            {
                do_motors(MODE, input_motor);
                do_vibrate_mini();
            }
            else if (cabinet_type == config.cannonboard.CABINET_UPRIGHT)
                do_vibrate_upright();
            else if (cabinet_type == config.cannonboard.CABINET_MINI)
                do_vibrate_mini();
            break;
    }
}

// ------------------------------------------------------------------------------------------------
// Digital Outputs
// ------------------------------------------------------------------------------------------------

void OOutputs::set_digital(uint8_t output)
{
    dig_out |= output;   
}

void OOutputs::clear_digital(uint8_t output)
{
    dig_out &= ~output;
}

// ------------------------------------------------------------------------------------------------
// Motor Diagnostics
// Source: 0x1885E
// ------------------------------------------------------------------------------------------------

bool OOutputs::diag_motor(int16_t input_motor, uint8_t hw_motor_limit, uint32_t packets)
{
    switch (motor_state)
    {
        // Initalize
        case STATE_INIT:
            col1 = 10;
            col2 = 27;
            ohud.blit_text_new(col1, 9, "LEFT LIMIT");
            ohud.blit_text_new(col1, 11, "RIGHT LIMIT");
            ohud.blit_text_new(col1, 13, "CENTRE");
            ohud.blit_text_new(col1, 16, "MOTOR POSITION");
            ohud.blit_text_new(col1, 18, "LIMIT B3 LEFT");
            ohud.blit_text_new(col1, 19, "LIMIT B4 CENTRE");
            ohud.blit_text_new(col1, 20, "LIMIT B5 RIGHT");
            counter          = COUNTER_RESET;
            motor_centre_pos = 0;
            motor_enabled    = true;
            motor_state = STATE_LEFT;
            break;

        case STATE_LEFT:
            diag_left(input_motor, hw_motor_limit);
            break;

        case STATE_RIGHT:
            diag_right(input_motor, hw_motor_limit);
            break;

        case STATE_CENTRE:
            diag_centre(input_motor, hw_motor_limit);
            break;

        case STATE_DONE:
            diag_done();
            break;
    }

    // Print Motor Position & Limit Switch
    ohud.blit_text_new(col2, 16, "  H", 0x80);
    ohud.blit_text_new(col2, 16, Utils::to_string(input_motor).c_str(), 0x80);
    ohud.blit_text_new(col2, 18, (hw_motor_limit & BIT_3) ? "ON " : "OFF ", 0x80);
    ohud.blit_text_new(col2, 19, (hw_motor_limit & BIT_4) ? "ON " : "OFF ", 0x80);
    ohud.blit_text_new(col2, 20, (hw_motor_limit & BIT_5) ? "ON " : "OFF ", 0x80);
    return motor_state == STATE_DONE;
}

void OOutputs::diag_left(int16_t input_motor, uint8_t hw_motor_limit)
{
    // If Right Limit Set, Move Left
    if (hw_motor_limit & BIT_5)
    {
        if (--counter >= 0)
        {
            hw_motor_control = MOTOR_LEFT;
            return;
        }
        // Counter Expired, Left Limit Still Not Reached
        else
            ohud.blit_text_new(col2, 9, "FAIL 1", 0x80);
    }
    // Left Limit Reached
    else if (hw_motor_limit & BIT_3)
    {
        ohud.blit_text_new(col2, 9, "  H", 0x80);
        ohud.blit_text_new(col2, 9, Utils::to_string(input_motor).c_str(), 0x80);
    }
    else
        ohud.blit_text_new(col2, 9, "FAIL 2", 0x80);

    counter          = COUNTER_RESET;
    motor_state      = STATE_RIGHT;
}


void OOutputs::diag_right(int16_t input_motor, uint8_t hw_motor_limit)
{
    if (motor_centre_pos == 0 && ((hw_motor_limit & BIT_4) == 0))
        motor_centre_pos = input_motor;
   
    // If Left Limit Set, Move Right
    if (hw_motor_limit & BIT_3)
    {
        if (--counter >= 0)
        {
            hw_motor_control = MOTOR_RIGHT; // Move Right
            return;
        }
        // Counter Expired, Right Limit Still Not Reached
        else
            ohud.blit_text_new(col2, 11, "FAIL 1", 0x80);
    }
    // Right Limit Reached
    else if (hw_motor_limit & BIT_5)
    {
        ohud.blit_text_new(col2, 11, "  H", 0x80);
        ohud.blit_text_new(col2, 11, Utils::to_string(input_motor).c_str(), 0x80);
    }
    else
    {
        ohud.blit_text_new(col2, 11, "FAIL 2", 0x80);
        motor_enabled = false;
        motor_state   = STATE_DONE;
        return;
    }

    motor_state  = STATE_CENTRE;
    counter      = COUNTER_RESET;
}


void OOutputs::diag_centre(int16_t input_motor, uint8_t hw_motor_limit)
{
    if (hw_motor_limit & BIT_4) 
    {
        if (--counter >= 0)
        {
            hw_motor_control = (counter <= COUNTER_RESET/2) ? MOTOR_RIGHT : MOTOR_LEFT; // Move Right
            return;
        }
        else
        {
            ohud.blit_text_new(col2, 13, "FAIL", 0x80);
        }  
    }
    else
    {
        ohud.blit_text_new(col2, 13, "  H", 0x80);
        ohud.blit_text_new(col2, 13, Utils::to_string((input_motor + motor_centre_pos) >> 1).c_str(), 0x86);
        hw_motor_control = MOTOR_OFF; // switch off
        counter          = 32;
        motor_state      = STATE_DONE;
    }
}

void OOutputs::diag_done()
{
    if (counter > 0)
        counter--;

    if (counter == 0)
        hw_motor_control = MOTOR_CENTRE;
}

// ------------------------------------------------------------------------------------------------
// Calibrate Motors
// ------------------------------------------------------------------------------------------------

bool OOutputs::calibrate_motor(int16_t input_motor, uint8_t hw_motor_limit, uint32_t packets)
{
    switch (motor_state)
    {
        // Initalize
        case STATE_INIT:
            col1 = 11;
            col2 = 25;
            ohud.blit_text_big(      2,  "MOTOR CALIBRATION");
            ohud.blit_text_new(col1, 10, "MOVE LEFT   -");
            ohud.blit_text_new(col1, 12, "MOVE RIGHT  -");
            ohud.blit_text_new(col1, 14, "MOVE CENTRE -");
            counter          = 25;
            motor_centre_pos = 0;
            motor_enabled    = true;
            motor_state++;
            break;

        // Just a delay to wait for the serial for safety
        case STATE_DELAY:
            if (--counter == 0 || packets > 60)
            {
                counter = COUNTER_RESET;
                motor_state++;
            }
            break;

        // Calibrate Left Limit
        case STATE_LEFT:
            calibrate_left(input_motor, hw_motor_limit);
            break;

        // Calibrate Right Limit
        case STATE_RIGHT:
            calibrate_right(input_motor, hw_motor_limit);
            break;

        // Return to Centre
        case STATE_CENTRE:
            calibrate_centre(input_motor, hw_motor_limit);
            break;

        // Clear Screen & Exit Calibration
        case STATE_DONE:
            calibrate_done();
            break;

        case STATE_EXIT:
            return true;
    }

    return false;
}

void OOutputs::calibrate_left(int16_t input_motor, uint8_t hw_motor_limit)
{
    // If Right Limit Set, Move Left
    if (hw_motor_limit & BIT_5)
    {
        if (--counter >= 0)
        {
            hw_motor_control = MOTOR_LEFT;
            return;
        }
        // Counter Expired, Left Limit Still Not Reached
        else
        {
            ohud.blit_text_new(col2, 10, "FAIL 1");
            motor_centre_pos = 0;
            limit_left       = input_motor; // Set Left Limit
            hw_motor_control = MOTOR_LEFT;  // Move Left
            counter          = COUNTER_RESET;
            motor_state      = STATE_RIGHT;
        }
    }
    // Left Limit Reached
    else if (hw_motor_limit & BIT_3)
    {
        ohud.blit_text_new(col2, 10, Utils::to_hex_string(input_motor).c_str(), 0x80);
        motor_centre_pos = 0;
        limit_left       = input_motor; // Set Left Limit
        hw_motor_control = MOTOR_LEFT;  // Move Left
        counter          = COUNTER_RESET; 
        motor_state      = STATE_RIGHT;
    }
    else
    {
        ohud.blit_text_new(col2, 10, "FAIL 2");
        ohud.blit_text_new(col2, 12, "FAIL 2");
        motor_enabled = false; 
        counter       = COUNTER_RESET;
        motor_state   = STATE_CENTRE;
    }
}

void OOutputs::calibrate_right(int16_t input_motor, uint8_t hw_motor_limit)
{
    if (motor_centre_pos == 0 && ((hw_motor_limit & BIT_4) == 0))
    {
        motor_centre_pos = input_motor;
    }
   
    // If Left Limit Set, Move Right
    if (hw_motor_limit & BIT_3)
    {
        if (--counter >= 0)
        {
            hw_motor_control = MOTOR_RIGHT; // Move Right
            return;
        }
        // Counter Expired, Right Limit Still Not Reached
        else
        {
            ohud.blit_text_new(col2, 12, "FAIL 1");
            limit_right  = input_motor;
            motor_state  = STATE_CENTRE;
            counter      = COUNTER_RESET;
        }
    }
    // Right Limit Reached
    else if (hw_motor_limit & BIT_5)
    {
        ohud.blit_text_new(col2, 12, Utils::to_hex_string(input_motor).c_str(), 0x80);
        limit_right   = input_motor; // Set Right Limit
        motor_state   = STATE_CENTRE;
        counter       = COUNTER_RESET;
    }
    else
    {
        ohud.blit_text_new(col2, 12, "FAIL 2");
        motor_enabled = false;
        motor_state   = STATE_CENTRE;
        counter       = COUNTER_RESET;
    }
}

void OOutputs::calibrate_centre(int16_t input_motor, uint8_t hw_motor_limit)
{
    bool fail = false;

    if (hw_motor_limit & BIT_4) 
    {
        if (--counter >= 0)
        {
            hw_motor_control = (counter <= COUNTER_RESET/2) ? MOTOR_RIGHT : MOTOR_LEFT; // Move Right
            return;
        }
        else
        {
            ohud.blit_text_new(col2, 14, "FAIL SW");
            fail = true;
            // Fall through to EEB6
        }  
    }
  
    // 0xEEB6:
    motor_centre_pos = (input_motor + motor_centre_pos) >> 1;
  
    int16_t d0 = limit_right - motor_centre_pos;
    int16_t d1 = motor_centre_pos  - limit_left;
  
    // set both to left limit
    if (d0 > d1)
        d1 = d0;

    d0 = d1;
  
    limit_left  = d0 - 6;
    limit_right = -d1 + 6;
    
    if (std::abs(motor_centre_pos - 0x80) > 0x20)
    {
        ohud.blit_text_new(col2, 14, "FAIL DIST");
        motor_enabled = false;
    }
    else if (!fail)
    {
        ohud.blit_text_new(col2, 14, Utils::to_hex_string(motor_centre_pos).c_str(), 0x80);
    }

    ohud.blit_text_new(13, 17, "TESTS COMPLETE!", 0x82);

    hw_motor_control = MOTOR_OFF; // switch off
    counter          = 90;
    motor_state      = STATE_DONE;
}

void OOutputs::calibrate_done()
{
    if (counter > 0)
        counter--;
    else
        motor_state = STATE_EXIT;
}

// ------------------------------------------------------------------------------------------------
// Moving Cabinet Code
// ------------------------------------------------------------------------------------------------

const static uint8_t MOTOR_VALUES[] = 
{
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3,
    2, 2, 3, 3, 4, 4, 5, 5, 2, 3, 4, 5, 6, 7, 7, 7
};

const static uint8_t MOTOR_VALUES_STATIONARY[] = 
{
    2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4
};

const static uint8_t MOTOR_VALUES_OFFROAD1[] = 
{
    0x8, 0x8, 0x5, 0x5, 0x8, 0x8, 0xB, 0xB, 0x8, 0x8, 0x4, 0x4, 0x8, 0x8, 0xC, 0xC, 
    0x8, 0x8, 0x3, 0x3, 0x8, 0x8, 0xD, 0xD, 0x8, 0x8, 0x2, 0x2, 0x8, 0x8, 0xE, 0xE,
};

const static uint8_t MOTOR_VALUES_OFFROAD2[] = 
{
    0x8, 0x5, 0x8, 0xB, 0x8, 0x5, 0x8, 0xB, 0x8, 0x4, 0x8, 0xC, 0x8, 0x4, 0x8, 0xC,
    0x8, 0x3, 0x8, 0xD, 0x8, 0x3, 0x8, 0xD, 0x8, 0x2, 0x8, 0xE, 0x8, 0x2, 0x8, 0xE,
};

const static uint8_t MOTOR_VALUES_OFFROAD3[] = 
{
    0x8, 0x5, 0x5, 0x8, 0xB, 0x8, 0x0, 0x8, 0x8, 0x4, 0x4, 0x8, 0xC, 0x8, 0x0, 0x8,
    0x8, 0x3, 0x3, 0x8, 0xD, 0x8, 0x0, 0x8, 0x8, 0x2, 0x2, 0x8, 0xE, 0x8, 0x0, 0x8,
};

const static uint8_t MOTOR_VALUES_OFFROAD4[] = 
{
    0x8, 0xB, 0xB, 0x8, 0x5, 0x8, 0x0, 0x8, 0x8, 0xC, 0xC, 0x8, 0x4, 0x8, 0x0, 0x8,
    0x8, 0xD, 0xD, 0x8, 0x3, 0x8, 0x0, 0x8, 0x8, 0xE, 0xE, 0x8, 0x2, 0x8, 0x0, 0x8,
};


// Process Motor Code.
// Note, that only the Deluxe Moving Motor Code is ported for now.
// Source: 0xE644
void OOutputs::do_motors(int MODE, int16_t input_motor)
{
    motor_x_change = -(input_motor - (MODE == MODE_FFEEDBACK ? CENTRE_POS : motor_centre_pos));

    if (!motor_enabled)
    {
        done();
        return;
    }

    // In-Game: Test for crash, skidding, whether car is moving
    if (outrun.game_state == GS_INGAME)
    {
        if (ocrash.crash_counter)
        {
            if ((oinitengine.car_increment >> 16) <= 0x14)
                car_stationary();
            else
                do_motor_crash();
        }
        else if (ocrash.skid_counter)
        {
            do_motor_crash();
        }
        else
        {
            if ((oinitengine.car_increment >> 16) <= 0x14)
            {
                if (!was_small_change)
                    done();
                else
                    car_stationary();
            }
            else
                car_moving(MODE);
        }
    }
    // Not In-Game: Act as though car is stationary / moving slow
    else
    {
        car_stationary();
    }
}

// Source: 0xE6DA
void OOutputs::car_moving(const int MODE)
{
    // Motor is currently moving
    if (motor_movement)
    {
        hw_motor_control = motor_control;
        adjust_motor();
        return;
    }
    
    // Motor is not currently moving. Setup new movement as necessary.
    if (oferrari.wheel_state != OFerrari::WHEELS_ON)
    {
        do_motor_offroad();
        return;
    }

    const uint16_t car_inc = oinitengine.car_increment >> 16;
    if (car_inc <= 0x64)                    speed = 0;
    else if (car_inc <= 0xA0)               speed = 1 << 3;
    else if (car_inc <= 0xDC)               speed = 2 << 3;
    else                                    speed = 3 << 3;

    if (oinitengine.road_curve == 0)         curve = 0;
    else if (oinitengine.road_curve <= 0x3C) curve = 2; // sharp curve
    else if (oinitengine.road_curve <= 0x5A) curve = 1; // gentle curve
    else                                     curve = 0;

    int16_t steering = oinputs.steering_adjust;
    steering += (movement_adjust1 + movement_adjust2 + movement_adjust3);
    steering >>= 2;
    movement_adjust3 = movement_adjust2;                   // Trickle down values
    movement_adjust2 = movement_adjust1;
    movement_adjust1 = oinputs.steering_adjust;

    // Veer Left
    if (steering >= 0)
    {
        steering = (steering >> 4) - 1;
        if (steering < 0)
        {
            car_stationary();
            return;
        }
                
        if (steering > 0)
            steering--;

        uint8_t motor_value = MOTOR_VALUES[speed + curve];

        if (MODE == MODE_FFEEDBACK)
        {
            hw_motor_control = motor_value + 8;
        }
        else
        {
            int16_t change = motor_x_change + (motor_value << 1);
            // Latch left movement
            if (change >= limit_left)
            {
                hw_motor_control   = MOTOR_CENTRE;
                motor_movement     = 1;
                motor_control      = 7;
                motor_change_latch = motor_x_change;
            }
            else
            {
                hw_motor_control = motor_value + 8;
            }
        }
        
        done();
    }
    // Veer Right
    else
    {
        steering = -steering;
        steering = (steering >> 4) - 1;
        if (steering < 0)
        {
            car_stationary();
            return;
        }

        if (steering > 0)
            steering--;

        uint8_t motor_value = MOTOR_VALUES[speed + curve];

        if (MODE == MODE_FFEEDBACK)
        {
            hw_motor_control = -motor_value + 8;
        }
        else
        {
            int16_t change = motor_x_change - (motor_value << 1);
            // Latch right movement
            if (change <= limit_right)
            {
                hw_motor_control   = MOTOR_CENTRE;
                motor_movement     = -1;
                motor_control      = 9;
                motor_change_latch = motor_x_change;
            }
            else
            {
                hw_motor_control = -motor_value + 8;
            }
        }
        
        done();
    }
}

// Source: 0xE822
void OOutputs::car_stationary()
{
    int16_t change = std::abs(motor_x_change);

    if (change <= 8)
    {
        if (!is_centered)
        {
            hw_motor_control = MOTOR_CENTRE;
            is_centered      = true;
        }
        else
        {
            hw_motor_control = MOTOR_OFF;
            is_centered      = false;
            done();
        }
    }
    else
    {
        int8_t motor_value = MOTOR_VALUES_STATIONARY[change >> 3];

        if (motor_x_change >= 0)
            motor_value = -motor_value;

        hw_motor_control = motor_value + 8;

        done();
    }
}

// Source: 0xE8DA
void OOutputs::adjust_motor()
{
    int16_t change = motor_change_latch; // d1
    motor_change_latch = motor_x_change;
    change -= motor_x_change;
    if (change < 0) 
        change = -change;

    // no movement
    if (change <= 2)
    {
        motor_movement = 0;
        is_centered    = true;
    }
    // moving right
    else if (motor_movement < 0)
    {
        if (++motor_control > 10)
            motor_control = 10;
    }
    // moving left
    else 
    {
        if (--motor_control < 6)
            motor_control = 6;
    }

    done();
}

// Adjust motor during crash/skid state
// Source: 0xE994
void OOutputs::do_motor_crash()
{
    if (oferrari.car_x_diff == 0)
        set_value(MOTOR_VALUES_OFFROAD1, 3);
    else if (oferrari.car_x_diff < 0)
        set_value(MOTOR_VALUES_OFFROAD4, 3);
    else
        set_value(MOTOR_VALUES_OFFROAD3, 3);
}

// Adjust motor when wheels are off-road
// Source: 0xE9BE
void OOutputs::do_motor_offroad()
{
    const uint8_t* table = (oferrari.wheel_state != OFerrari::WHEELS_OFF) ? MOTOR_VALUES_OFFROAD2 : MOTOR_VALUES_OFFROAD1;

    const uint16_t car_inc = oinitengine.car_increment >> 16;
    uint8_t index;
    if (car_inc <= 0x32)      index = 0;
    else if (car_inc <= 0x50) index = 1;
    else if (car_inc <= 0x6E) index = 2;
    else                      index = 3;

    set_value(table, index);
}

void OOutputs::set_value(const uint8_t* table, uint8_t index)
{
    hw_motor_control = table[(index << 3) + (counter & 7)];
    counter++;
    done();
}

// Source: 0xE94E
void OOutputs::done()
{
    if (std::abs(motor_x_change) <= 8)
    {
        was_small_change = true;
        motor_control    = MOTOR_CENTRE;
    }
    else
    {
        was_small_change = false;
    }
}

// Send output commands to motor hardware
// This is the equivalent to writing to register 0x140003
void OOutputs::motor_output(uint8_t cmd)
{
    if (cmd == MOTOR_OFF || cmd == MOTOR_CENTRE)
        return;

    int8_t force = 0;

    if (cmd < MOTOR_CENTRE)      // left
        force = cmd - 1;
    else if (cmd > MOTOR_CENTRE) // right
        force = 15 - cmd;

    forcefeedback::set(cmd, force);
}

// ------------------------------------------------------------------------------------------------
// Deluxe Upright: Steering Wheel Movement
// ------------------------------------------------------------------------------------------------

// Deluxe Upright: Vibration Enable Table. 4 Groups of vibration values.
const static uint8_t VIBRATE_LOOKUP[] = 
{
    // SLOW SPEED --------   // MEDIUM SPEED ------
    1, 0, 0, 0, 1, 0, 0, 0,  1, 1, 0, 0, 1, 1, 0, 0,
    // FAST SPEED --------   // VERY FAST SPEED ---
    1, 1, 1, 0, 1, 1, 1, 0,  1, 1, 1, 1, 1, 1, 1, 1,
};

// Source: 0xEAAA
void OOutputs::do_vibrate_upright()
{
    if (outrun.game_state != GS_INGAME)
    {
        clear_digital(D_MOTOR);
        return;
    }

    const uint16_t speed = oinitengine.car_increment >> 16;
    uint16_t index = 0;

    // Car Crashing: Diable Motor once speed below 10
    if (ocrash.crash_counter)
    {
        if (speed <= 10)
        {
            clear_digital(D_MOTOR);
            return;
        }
    }
    // Car Normal
    else if (!ocrash.skid_counter)
    {
        // 0xEAE2: Disable Vibration once speed below 30 or wheels on-road
        if (speed < 30 || oferrari.wheel_state == OFerrari::WHEELS_ON)
        {
            clear_digital(D_MOTOR);
            return;
        }

        // 0xEAFC: Both wheels off-road. Faster the car speed, greater the chance of vibrating
        if (oferrari.wheel_state == OFerrari::WHEELS_OFF)
        {
            if (speed > 220)      index = 3;
            else if (speed > 170) index = 2;
            else if (speed > 120) index = 1;
        }
        // 0xEB38: One wheel off-road. Faster the car speed, greater the chance of vibrating
        else
        {
            if (speed > 270)      index = 3;
            else if (speed > 210) index = 2;
            else if (speed > 150) index = 1;
        }

        if (VIBRATE_LOOKUP[ (vibrate_counter & 7) + (index << 3) ])
            set_digital(D_MOTOR);
        else
            clear_digital(D_MOTOR);

        vibrate_counter++;
        return;
    }
    // 0xEB68: Car Crashing or Skidding
    if (speed > 140)      index = 3;
    else if (speed > 100) index = 2;
    else if (speed > 60)  index = 1;

    if (VIBRATE_LOOKUP[ (vibrate_counter & 7) + (index << 3) ])
        set_digital(D_MOTOR);
    else
        clear_digital(D_MOTOR);

    vibrate_counter++;
}

// ------------------------------------------------------------------------------------------------
// Mini Upright: Steering Wheel Movement
// ------------------------------------------------------------------------------------------------

void OOutputs::do_vibrate_mini()
{
    if (outrun.game_state != GS_INGAME)
    {
        clear_digital(D_MOTOR);
        return;
    }

    const uint16_t speed = oinitengine.car_increment >> 16;
    uint16_t index = 0;

    // Car Crashing: Diable Motor once speed below 10
    if (ocrash.crash_counter)
    {
        if (speed <= 10)
        {
            clear_digital(D_MOTOR);
            return;
        }
    }
    // Car Normal
    else if (!ocrash.skid_counter)
    {
        if (speed < 10 || oferrari.wheel_state == OFerrari::WHEELS_ON)
        {
            clear_digital(D_MOTOR);
            return;
        }  

        if (speed > 140)      index = 5;
        else if (speed > 100) index = 4;
        else if (speed > 60)  index = 3;
        else if (speed > 20)  index = 2;
        else                  index = 1;

        if (index > vibrate_counter)
        {
            vibrate_counter = 0;
            clear_digital(D_MOTOR);
        }
        else
        {
            vibrate_counter++;
            set_digital(D_MOTOR);
        }
        return;
    }

    // 0xEC7A calc_crash_skid:
    if (speed > 90)      index = 4;
    else if (speed > 70) index = 3;
    else if (speed > 50) index = 2;
    else if (speed > 30) index = 1;
    if (index > vibrate_counter)
    {
        vibrate_counter = 0;
        clear_digital(D_MOTOR);
    }
    else
    {
        vibrate_counter++;
        set_digital(D_MOTOR);
    }
}

// ------------------------------------------------------------------------------------------------
// Coin Chute Output
// Source: 0x6F8C
// ------------------------------------------------------------------------------------------------

void OOutputs::coin_chute_out(CoinChute* chute, bool insert)
{
    // Initalize counter if coin inserted
    chute->counter[2] = insert ? 1 : 0;

    if (chute->counter[0])
    {
        if (--chute->counter[0] != 0)
            return;
        chute->counter[1] = 6;
        clear_digital(chute->output_bit);
    }
    else if (chute->counter[1])
    {
        chute->counter[1]--;
    }
    // Coin first inserted. Called Once. 
    else if (chute->counter[2])
    {
        chute->counter[2]--;
        chute->counter[0] = 6;
        set_digital(chute->output_bit);
    }
}