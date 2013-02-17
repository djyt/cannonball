/***************************************************************************
    Process Outputs.
    
    - Only the Deluxe Moving Motor Code is ported for now.
    - This is used by the force-feedback system.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

// Port code at 0xE644

//REGISTERS:00140001 hw_motor_limit: ds.b 1                                      ; DATA XREF: sub_ED46:loc_ED56r
//REGISTERS:00140001                                                             ; sub_ED46:loc_EDD4r ...
//REGISTERS:00140001                                                             ; Bit 3 = Set to indicate left limit reached
//REGISTERS:00140001                                                             ; Bit 4 = Set to indicate centre reached
//REGISTERS:00140001                                                             ; Bit 5 = Set to indicate right limit reached
//REGISTERS:00140001                                                             ;
//REGISTERS:00140002                 ds.b 1
//REGISTERS:00140003 hw_motor_ctrl:  ds.b 1                                      ; DATA XREF: DoMotors+9Ew
//REGISTERS:00140003                                                             ; DoMotors+18Cw ...
//REGISTERS:00140003                                                             ; Move motor
//REGISTERS:00140003                                                             ;
//REGISTERS:00140003                                                             ; 0 = Switch off?
//REGISTERS:00140003                                                             ; 5 = Left
//REGISTERS:00140003                                                             ; 8 = Centre
//REGISTERS:00140003                                                             ; B = Right

  //if (a==0x140001) {
	 // // Motor Limit Status
	 // // return 0x2d; 
	 // if (OutAxis[0]<0x03) return 0x0d; // LEFT LIMIT REACHED
	 // if (OutAxis[0]>0xfc) return 0x20; // RIGHT LIMIT REACHED
	 // return 0x2d;
  //}

  //if (a==0x140031)
  //{
  //  switch (AnalogSelect)
  //  {
  //    case 0x0: return OutAxis[0];
  //    case 0x4: return OutAxis[1];
  //    case 0x8: return OutAxis[2];
  //    case 0xc: return OutAxis[0]; // Motor Status x-pos
  //  }

#include "engine/outrun.hpp"
#include "engine/ocrash.hpp"
#include "engine/oferrari.hpp"
#include "engine/oinputs.hpp"
#include "engine/ooutputs.hpp"
#include "directx/ffeedback.hpp"

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

OOutputs::OOutputs(void)
{
}

OOutputs::~OOutputs(void)
{
}


// Initalize Moving Cabinet Motor
// Source: 0xECE8
void OOutputs::init()
{
    motor_state        = 0;
    motor_enabled      = true;
    hw_motor_control   = 0;

    motor_control      = 0;
    motor_movement     = 0;
    is_centered        = false;
    motor_change_latch = 0;
    speed              = 0;
    curve              = 0;
    counter            = 0;
    was_small_change   = false;
    movement_adjust1   = 0;
    movement_adjust2   = 0;
    movement_adjust3   = 0;
}

// Process Motor Code.
// Note, that only the Deluxe Moving Motor Code is ported for now.
// Source: 0xE644
void OOutputs::do_motors()
{
    // Simulate read to Analog Select Register (0x140031)
    // Normally this would be done on H-Blank
    // But instead we return the x-position of the wheel
    input_motor = oinputs.input_steering;
    motor_x_change = -(input_motor - input_motor_old);

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
            if (oinitengine.car_increment <= 0x14)
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
            if (oinitengine.car_increment <= 0x14)
            {
                if (!was_small_change)
                    done();
                else
                    car_stationary();
            }
            else
                car_moving();
        }
    }
    // Not In-Game: Act as though car is stationary / moving slow
    else
    {
        car_stationary();
    }
}


// Source: 0xE6DA
void OOutputs::car_moving()
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

    if (oinitengine.car_increment <= 0x64)      speed = 0;
    else if (oinitengine.car_increment <= 0xA0) speed = 1 << 3;
    else if (oinitengine.car_increment <= 0xDC) speed = 2 << 3;
    else                                        speed = 3 << 3;

    if (oinitengine.road_curve == 0)            curve = 0;
    else if (oinitengine.road_curve <= 0x3C)    curve = 2; // sharp curve
    else if (oinitengine.road_curve <= 0x5A)    curve = 1; // gentle curve
    else                                        curve = 0;

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
        int16_t change = motor_x_change + (motor_value << 1);

        // Latch left movement
        if (change >= LEFT_LIMIT)
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
        int16_t change = motor_x_change - (motor_value << 1);

        // Latch right movement
        if (change <= RIGHT_LIMIT)
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

        if (motor_x_change < 0)
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

    // Finally output the info to the force feedback handling class
    motor_output(hw_motor_control);
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

    uint8_t index;
    if (oinitengine.car_increment <= 0x32)      index = 0;
    else if (oinitengine.car_increment <= 0x50) index = 1;
    else if (oinitengine.car_increment <= 0x6E) index = 2;
    else                                        index = 3;

    set_value(table, index);
}

void OOutputs::set_value(const uint8_t* table, uint8_t index)
{
    hw_motor_control = table[(index << 3) + (counter & 7)];
    counter++;

    done();
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