/***************************************************************************
    Process Outputs.
    
    - Only the Deluxe Moving Motor Code is ported for now.
    - This is used by the force-feedback haptic system.

    One thing to note is that this code was originally intended to drive
    a moving hydraulic cabinet, not to be mapped to a haptic device.

    Therefore, it's not perfect when used in this way, but the results
    aren't bad :)
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "stdint.hpp"

struct CoinChute
{
    // Coin Chute Counters
    uint8_t counter[3];
    // Output bit
    uint8_t output_bit;
};

class OOutputs
{
public:
    
    const static int MODE_FFEEDBACK = 0;
    const static int MODE_CABINET   = 1;

    // Hardware Motor Control:
    // 0 = Switch off
    // 5 = Left
    // 8 = Centre
    // B = Right
    uint8_t hw_motor_control;

    // Digital Outputs
    enum
    {
        D_EXT_MUTE   = 0x01, // bit 0 = External Amplifier Mute Control
        D_BRAKE_LAMP = 0x02, // bit 1 = brake lamp
        D_START_LAMP = 0x04, // bit 2 = start lamp
        D_COIN1_SUCC = 0x08, // bit 3 = Coin successfully inserted - Chute 2
        D_COIN2_SUCC = 0x10, // bit 4 = Coin successfully inserted - Chute 1
        D_MOTOR      = 0x20, // bit 5 = steering wheel central vibration
        D_UNUSED     = 0x40, // bit 6 = ?
        D_SOUND      = 0x80, // bit 7 = sound enable
    };
    uint8_t dig_out;

    CoinChute chute1, chute2;

    OOutputs(void);
    ~OOutputs(void);

    void init();
    bool diag_motor(int16_t input_motor, uint8_t hw_motor_limit, uint32_t packets);
    bool calibrate_motor(int16_t input_motor, uint8_t hw_motor_limit, uint32_t packets);
    void tick(const int MODE, int16_t input_motor, int16_t cabinet_type = -1);
    void set_digital(uint8_t);
    void clear_digital(uint8_t);
    void coin_chute_out(CoinChute* chute, bool insert);

private:
    const static uint16_t STATE_INIT   = 0;
    const static uint16_t STATE_DELAY  = 1;
    const static uint16_t STATE_LEFT   = 2;
    const static uint16_t STATE_RIGHT  = 3;
    const static uint16_t STATE_CENTRE = 4;
    const static uint16_t STATE_DONE   = 5;
    const static uint16_t STATE_EXIT   = 6;

    // Calibration Counter
    const static int COUNTER_RESET = 300;

    const static uint8_t MOTOR_OFF    = 0;
    const static uint8_t MOTOR_RIGHT  = 0x5;
    const static uint8_t MOTOR_CENTRE = 0x8;
    const static uint8_t MOTOR_LEFT   = 0xB;
    

    // These are calculated during startup in the original game.
    // Here we just hardcode them, as the motor init code isn't ported.
    const static uint8_t CENTRE_POS    = 0x80;
    const static uint8_t LEFT_LIMIT    = 0xC1;
    const static uint8_t RIGHT_LIMIT   = 0x3C;

    // Motor Limit Values. Calibrated during startup.
    int16_t limit_left;
    int16_t limit_right;

    // Motor Centre Position. (We Fudge this for Force Feedback wheel mode.)
    int16_t motor_centre_pos;

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
    int16_t vibrate_counter;
    // 0x20: Last Motor X_Change > 8. No need to adjust further.
    bool was_small_change;
    // 0x22: Adjusted movement value based on steering 1
    int16_t movement_adjust1;
    // 0x24: Adjusted movement value based on steering 2
    int16_t movement_adjust2;
    // 0x26: Adjusted movement value based on steering 3
    int16_t movement_adjust3;

    // Counter control for motor tests
    int16_t counter;

    // Columns for output
    uint16_t col1, col2;

    void diag_left(int16_t input_motor, uint8_t hw_motor_limit);
    void diag_right(int16_t input_motor, uint8_t hw_motor_limit);
    void diag_centre(int16_t input_motor, uint8_t hw_motor_limit);
    void diag_done();

    void calibrate_left(int16_t input_motor, uint8_t hw_motor_limit);
    void calibrate_right(int16_t input_motor, uint8_t hw_motor_limit);
    void calibrate_centre(int16_t input_motor, uint8_t hw_motor_limit);
    void calibrate_done();

    void do_motors(const int MODE, int16_t input_motor);
    void car_moving(const int MODE);
    void car_stationary();
    void adjust_motor();
    void do_motor_crash();
    void do_motor_offroad();
    void set_value(const uint8_t*, uint8_t);
    void done();
    void motor_output(uint8_t cmd);

    void do_vibrate_upright();
    void do_vibrate_mini();
};