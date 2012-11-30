/***************************************************************************
    Collision & Crash Code. 
    
    There are two types of collision: Scenery & Traffic.
    
    1/ Traffic: The Ferrari will spin after a collision.
    2/ Scenery: There are three types of scenery collision:
       - Low speed bump. Car rises slightly in the air and stalls.
       - Mid speed spin. Car spins and slides after collision.
       - High speed flip. If slightly slower, car rolls into screen.
         Otherwise, grows towards screen and vanishes
         
    Known Issues With Original Code:
    - Passenger sprites flicker if they land moving in the water on Stage 1
    
    The Ferrari sprite is used differently by the crash code.
    As there's only one of them, I've rolled the additional variables into
    this class. 
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.hpp"

class OCrash
{
public:
    // Reference to sprite used by crash code
    oentry* spr_ferrari;
    oentry* spr_shadow;
    oentry* spr_pass1;
    oentry* spr_pass1s;
    oentry* spr_pass2;
    oentry* spr_pass2s;

    // Default value to reset skid counter to on collision
    const static uint8_t SKID_RESET = 0x14;

    // Maximum value to allow skid reset to be set to during collision
    const static uint8_t SKID_MAX = 0x1E;

    // Amount to adjust car x position by when skidding
    const static uint8_t SKID_X_ADJ = 0x18;

    //Crash State [Investigate Further]
    //
    // 0 = No crash
    // 1 = Collision with object. Init Spin if medium speed collision.
    // 2 = Flip Car.
    // 3 = Slide Car. Trigger Smoke Cloud.
    // 4 = Horizontally Flip Car. Trigger Smoke Cloud.
    // 5 = SPIN: Remove Smoke From Spin. Girl Points Finger.
    //     FLIP: Flip Animation Done.
    // 6 = Pan Camera To Track Centre
    // 7 = Camera Repositioned. Ready For Restart
    //
    // (Note that hitting another vehicle and skidding does not affect the crash state)
    int8_t crash_state;

    // Skid Counter. Set On Collision With Another Vehicle Only.
    //
    // If positive, skid to the left.
    // If negative, skid to the right.
    int16_t skid_counter;
    int16_t skid_counter_bak;

    // Spin Control 1 - SPIN only
    //
    // 0 = No Spin
    // 1 = Init Spin Car
    // 2 = Spin In Progress
    uint8_t spin_control1;

    uint8_t spin_control2;

    // Increments on a per collision basis (Follows on from collision_sprite being set)
    //
    // Used to cycle passenger animations after a crash
    //
    // coll_count1 != coll_count2 = Crash Subroutines Not Enabled
    // coll_count1 == coll_count2 = Crash Subroutines Enabled
    int16_t coll_count1;
    int16_t coll_count2;

    // Counter that increments per-frame during a crash scenario
    int16_t crash_counter;

    // Denotes the spin/flip number following the crash.
    //
    // 0 = No crash has taken place yet
    // 1 = Crash has taken place this level
    // 2 = Crash. First Spin/Flip. 
    // 3 = Crash. Second Spin/Flip.
    // 4 = Crash. Third Spin/Flip.
    int16_t crash_spin_count;

    // Crash Sprite Z Position
    int16_t crash_z;

    OCrash(void);
    ~OCrash(void);
    void init(oentry* f, oentry* s, oentry* p1, oentry* p1s, oentry* p2, oentry* p2s);
    void enable();
    void clear_crash_state();
    void tick();

private:

    // This is the rolled Ferrari sprite, which is configured differently for
    // the crash code. 
    // The offsets indicate the original offsets in memory.

    //+1E [Word] Spins/Flips Remaining
    int16_t spinflipcount1;
    //+22 [Word] Crash Spin/Flip Count Copy
    int16_t spinflipcount2;
    //+24 [Word] Crash Slide Value (After Spin/Flip etc.)
    int16_t slide;
    //+26 [Word] Frame (actually an index into the Sprite format below)
    int16_t frame;
    //+28 [Long] Address of animation sequence (frame address, palette info etc.)
    uint32_t addr;
    //+2C [Word] Camera Pan X Target (for repositioning after crash)
    int16_t camera_x_target;
    //+2E [Word] Camera Pan Increment
    int16_t camera_xinc;
    //+30 [Word] Index into movement lookup table (to set y position of car during low speed bump)
    int16_t lookup_index;
    //+32 [Word] Frame to restore car to after bump routine
    int16_t frame_restore;
    //+34 [Word] Used as a shift value to change y position during shunt
    int16_t shift;
    //+36 [Word] Flip Only: 0 = Fast Crash. 1 = Slow Crash.
    int16_t crash_speed;
    //+38 [Word] Crash Z Increment (How much to change Crash Z Per Tick)
    int16_t crash_zinc;
    //+3A [Word] 0 = RHS, 1 = LHS?
    int16_t crash_side;

    // Passenger Frame to use during spin
    int16_t spin_pass_frame;

    int8_t crash_type;
    enum { CRASH_BUMP = 0, CRASH_SPIN = 1, CRASH_FLIP = 2 };

    // Delay counter after crash. 
    // Show animation (e.g. girl pointing finger, before car is repositioned)
    int16_t crash_delay;

    void do_crash();
    void spin_switch(const uint16_t);
    void crash_switch();

    void init_collision();
    void do_collision();
    void end_collision();

    void do_bump();
    void do_car_flip();
    void init_finger(uint32_t);
    void trigger_smoke();
    void post_flip_anim();
    void pan_camera();

    void init_spin1();
    void init_spin2();
    void collide_slow();
    void collide_med();
    void collide_fast();

    void done(oentry*);

    void do_shadow(oentry*, oentry*);

    // Pointers to functions for crash code
    void (OCrash::*function_pass1)(oentry*);
    void (OCrash::*function_pass2)(oentry*);

    void do_crash_passengers(oentry*);
    void flip_start(oentry*);

    void crash_pass1(oentry*);
    void crash_pass2(oentry*);
    void crash_pass_flip(oentry*);

    void pass_flip(oentry*);
    void pass_situp(oentry*);
    void pass_turnhead(oentry*);
};

extern OCrash ocrash;