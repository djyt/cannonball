#pragma once

#include "stdint.hpp"

class OOutputs
{
public:
    
    // Hardware Motor Control:
    // 0 = Switch off
    // 5 = Left
    // 8 = Centre
    // B = Right
    uint8_t hw_motor_control;

    OOutputs(void);
    ~OOutputs(void);

    void init();
    void do_motors();

private:
    const static uint8_t MOTOR_OFF    = 0;
    const static uint8_t MOTOR_CENTRE = 8;

    // These are calculated during startup in the original game.
    // Here we just hardcode them, as the motor init code isn't ported.
    const static int8_t LEFT_LIMIT    = -14;
    const static int8_t RIGHT_LIMIT   = 14;

    // Motor Value, representing the x-position of the cabinet.
    // We fudge this and just use the x-position of the steering wheel
    int16_t input_motor;
    int16_t input_motor_old;

    // Difference between input_motor and input_motor_old
    int16_t motor_x_change;

    uint16_t motor_state;
    bool motor_enabled;

    // 0x11: Motor Control Value
    int8_t motor_control;
    // 0x12: Movement (1 = Left, -1 = Right, 0 = None)
    int8_t motor_movement;
    // 0x14: Is Motor Centered
    bool is_centered;
    // 0x16: Motor X Change Latch
    int16_t motor_change_latch;
    // 0x18: Speed
    int16_t speed;
    // 0x1A: Road Curve
    int16_t curve;
    // 0x1E: Increment counter to index motor table for off-road/crash
    int16_t counter;
    // 0x20: Last Motor X_Change > 8. No need to adjust further.
    bool was_small_change;
    // 0x22: Adjusted movement value based on steering 1
    int16_t movement_adjust1;
    // 0x24: Adjusted movement value based on steering 2
    int16_t movement_adjust2;
    // 0x26: Adjusted movement value based on steering 3
    int16_t movement_adjust3;

    void motor_output(uint8_t cmd);
    void car_moving();
    void car_stationary();
    void adjust_motor();
    void do_motor_crash();
    void do_motor_offroad();
    void set_value(const uint8_t*, uint8_t);
    void done();
};