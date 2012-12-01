/***************************************************************************
    Ferrari Rendering & Handling Code.
       
    Much of the handling code is very messy. As such, the translated code 
    isn't great as I tried to focus on accuracy rather than refactoring.
    
    A good example of the randomness is a routine I've named
      do_sound_score_slip()
    which performs everything from updating the score, setting the audio
    engine tone, triggering smoke effects etc. in an interwoven fashion.
    
    The Ferrari sprite has different properties to other game objects
    As there's only one of them, I've rolled the additional variables into
    this class. 
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.hpp"
#include "osprites.hpp"

class OFerrari
{
public:
    // Ferrari Sprite Object
    oentry *spr_ferrari;

    // Passenger 1 Sprite Object
    oentry *spr_pass1;

    // Passenger 2 Sprite Object
    oentry *spr_pass2;

    // Ferrari Shadow Sprite Object
    oentry *spr_shadow;

	// -------------------------------------------------------------------------
	// Main Switch Variables
	// -------------------------------------------------------------------------

    enum
    {
        // Initialise Intro Animation Sequences
        FERRARI_SEQ1 = 0,

        // Tick Intro Animation Sequences
        FERRARI_SEQ2 = 1,

        // Initialize In-Game Logic
        FERRARI_INIT = 2,

        // Tick In-Game Logic
        FERRARI_LOGIC = 3,
        
        // Ferrari End Sequence Logic
        FERRARI_END_SEQ = 4,
    };

    // Which routine is in use
    uint8_t state; 

    // Unused counter. Implemented on original game so could be useful for debug.
    uint16_t counter;

    int16_t steering_old;
    bool car_ctrl_active;
    
    // Car State
    //
    // -1 = Animation Sequence (Crash / Drive In)
    // 0  = Normal
    // 1  = Smoke from wheels
    int8_t car_state;

    enum { CAR_ANIM_SEQ = -1, CAR_NORMAL = 0, CAR_SMOKE = 1};

    // Auto breaking for end sequence
    bool auto_brake;

    // Torque table index lookup
    //
    // 00 = Start line only
    // 10 = Low gear
    // 1F = High gear
    //
    // Increments between the values
    //
    // Gets set based on what gear we're in 
    uint8_t torque_index; 
    int16_t torque;
    int32_t revs;

    // Rev Shift Value. Normal = 1.
    // Higher values result in reaching higher revs faster!
    uint8_t rev_shift;

    // State of car wheels
    //
    // 0 = On Road
    // 1 = Left Wheel Off-Road
    // 2 = Right Wheel Off-Road
    // 3 = Both Wheels Off-Road
    uint8_t wheel_state;

    enum
    {
        WHEELS_ON = 0,
        WHEELS_LEFT_OFF = 1,
        WHEELS_RIGHT_OFF = 2,
        WHEELS_OFF = 3
    };

    // Wheel Traction
    //
    // 0 = Both Wheels Have Traction
    // 1 = One Wheel Has Traction
    // 2 = No Wheels Have Traction
    uint8_t wheel_traction;

    enum
    {
        TRACTION_ON = 0,
        TRACTION_HALF = 1,
        TRACTION_OFF = 2,
    };

    // Ferrari is slipping/skidding either after collision or round bend
    uint16_t is_slipping;

    // Slip Command Sent To Sound Hardware
    uint8_t slip_sound;

    // Stores previous value of car_increment
    uint16_t car_inc_old;

    // Difference between car_x_pos and car_x_old
    int16_t car_x_diff;

	// -------------------------------------------------------------------------
	// Engine Stop Flag
	// -------------------------------------------------------------------------

    // Flag set when switching back to in-game engine, to be used with revs_post_stop
    // This is used to adjust the rev boost when returning to game
    int16_t rev_stop_flag;

    // Rev boost when we switch back to ingame engine and hand user control. 
    // Set by user being on revs before initialization.
    int16_t revs_post_stop;

    int16_t acc_post_stop;

	// -------------------------------------------------------------------------
	// Engine Sounds. Probably needs to be moved
	// -------------------------------------------------------------------------

    // Sound: Adjusted rev value (to be used to set pitch sound fx)
    uint16_t rev_pitch1;

    uint16_t rev_pitch2;

	// -------------------------------------------------------------------------
	// Ferrari Specific Values
	// -------------------------------------------------------------------------

    // *22 [Word] AI Curve Counter. Increments During Curve. Resets On Straight.
    int16_t sprite_ai_counter;

    // *24 [Word] AI Curve Value. 0x96 - curve_next.
    int16_t sprite_ai_curve;

    // *26 [Word] AI X Position Adjustment
    int16_t sprite_ai_x;

    // *28 [Word] AI Steering Adjustment
    int16_t sprite_ai_steer;
    
    // *2A [Word] Car X Position Backup
    int16_t sprite_car_x_bak;

    // *2C [Word] Wheel State
    int16_t sprite_wheel_state;

    // *2E [Word] Ferrari Slipping (Copy of slip counter)
    int16_t sprite_slip_copy;

    // *39 [Byte] Wheel Palette Offset
    int8_t wheel_pal;

    // *3A [Word] Passenger Y Offset
    int16_t sprite_pass_y;

    // *3C [Word] Wheel Frame Counter Reset
    int16_t wheel_frame_reset;

    // *3E [Word] Wheel Frame Counter Reset
    int16_t wheel_counter;


    OFerrari(void);
    ~OFerrari(void);
    void init(oentry*, oentry*, oentry*, oentry*);
    void reset_car();
    void init_ingame();
    void tick();
    void set_ferrari_x();
    void set_ferrari_bounds();
    void check_wheels();
    void set_curve_adjust();
    void draw_shadow();
    void move();
    void do_sound_score_slip();
    void shake();
    void do_skid();
    
private:
    // Max speed of car
    const static uint32_t MAX_SPEED = 0x1260000;

    // Car Base Increment, For Movement
    const static uint32_t CAR_BASE_INC = 0x12F;

    // Maximum distance to allow car to stray from road
    const static uint16_t OFFROAD_BOUNDS = 0x1F4;

    // Used by set_car_x
    int16_t road_width_old;

	// -------------------------------------------------------------------------
	// Controls
	// -------------------------------------------------------------------------
    
    int16_t accel_value;
    int16_t accel_value_bak;
    int16_t brake_value;
    bool gear_value;
    bool gear_bak;

    // Trickle down adjusted acceleration values
    int16_t acc_adjust1;
    int16_t acc_adjust2;
    int16_t acc_adjust3;

    // Trickle down brake values
    int16_t brake_adjust1;
    int16_t brake_adjust2;
    int16_t brake_adjust3;

    // Calculated brake value to subtract from acc_burst.
    int32_t brake_subtract;

    // Counter. When enabled, acceleration disabled
    int8_t gear_counter;

    // Previous rev adjustment (stored)
    int32_t rev_adjust;
    
	// -------------------------------------------------------------------------
	// Smoke
	// -------------------------------------------------------------------------

    // Counter for smoke after changing gear. Values over 0 result in smoke
    int16_t gear_smoke;

    // Similar to above
    int16_t gfx_smoke;

    // Set to -1 when car sharply corners and player is steering into direction of corner
    int8_t cornering;
    int8_t cornering_old;

    static const uint16_t torque_lookup[];
    static const uint8_t rev_inc_lookup[];

    void logic();
    void ferrari_normal();
    void setup_ferrari_sprite();
    void setup_ferrari_bonus_sprite();
    void init_end_seq();
    void do_end_seq();
    void tick_engine_disabled(int32_t&);
    void set_ferrari_palette();
    void set_passenger_sprite(oentry*);
    void set_passenger_frame(oentry*);
    void car_acc_brake();
    void do_gear_torque(int16_t&);
    void do_gear_low(int16_t&);
    void do_gear_high(int16_t&);
    int32_t tick_gear_change(int16_t);
    int32_t get_speed_inc_value(uint16_t, uint32_t);
    int32_t get_speed_dec_value(uint16_t);
    void set_brake_subtract();
    void finalise_revs(int32_t&, int32_t);
    void convert_revs_speed(int32_t, int32_t&);
    void update_road_pos();
    int32_t tick_smoke();
    void set_wheels(uint8_t);
};

extern OFerrari oferrari;