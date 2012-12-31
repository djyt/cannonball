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

#include "engine/outils.hpp"
#include "engine/ocrash.hpp"

OCrash ocrash;

OCrash::OCrash(void)
{
}


OCrash::~OCrash(void)
{
}

void OCrash::init(oentry* f, oentry* s, oentry* p1, oentry* p1s, oentry* p2, oentry* p2s)
{
    spr_ferrari = f;
    spr_shadow  = s;
    spr_pass1   = p1;
    spr_pass1s  = p1s;
    spr_pass2   = p2;
    spr_pass2s  = p2s;

    // Setup function pointers for passenger sprites
    function_pass1 = &OCrash::do_crash_passengers;
    function_pass2 = &OCrash::do_crash_passengers;
}

void OCrash::enable()
{
    // This is called multiple times, so need this check in place
    if (spr_ferrari->control & OSprites::ENABLE) 
        return;

    spr_ferrari->control |= OSprites::ENABLE;
    
    // Reset all corresponding variables
    spinflipcount1 = 0;
    spinflipcount2 = 0;
    slide = 0;
    frame = 0;
    addr = 0;
    camera_x_target = 0;
    camera_xinc = 0;
    lookup_index = 0;
    frame_restore = 0;
    shift = 0;
    crash_speed = 0;
    crash_zinc = 0;
    crash_side = 0;

    spr_ferrari->counter = 0;
}

// Source: 0x1128
void OCrash::clear_crash_state()
{
    spin_control1 = 0;
    coll_count1 = 0;
    coll_count2 = 0;
    olevelobjs.collision_sprite = 0;
    crash_counter = 0;
    crash_state = 0;
    crash_z = 0;
    spin_pass_frame = 0;
    crash_spin_count = 0;
    crash_delay = 0;
    crash_type = 0;
}

void OCrash::tick()
{
    // Do Ferrari
    if (spr_ferrari->control & OSprites::ENABLE)
        if (outrun.tick_frame) do_crash();
        else osprites.do_spr_order_shadows(spr_ferrari);

    // Do Car Shadow
    if (spr_shadow->control & OSprites::ENABLE)
        if (outrun.tick_frame) do_shadow(spr_ferrari, spr_shadow);
        else osprites.do_spr_order_shadows(spr_shadow);

    // Do Passenger 1
    if (spr_pass1->control & OSprites::ENABLE)
        if (outrun.tick_frame) ((ocrash).*(function_pass1))(spr_pass1);
        else osprites.do_spr_order_shadows(spr_pass1);

    // Do Passenger 1 Shadow
    if (spr_pass1s->control & OSprites::ENABLE)
        if (outrun.tick_frame) do_shadow(spr_pass1, spr_pass1s);
        else osprites.do_spr_order_shadows(spr_pass1s);

    // Do Passenger 2
    if (spr_pass2->control & OSprites::ENABLE)
        if (outrun.tick_frame) ((ocrash).*(function_pass2))(spr_pass2);
        else osprites.do_spr_order_shadows(spr_pass2);

    // Do Passenger 2 Shadow
    if (spr_pass2s->control & OSprites::ENABLE)
        if (outrun.tick_frame) do_shadow(spr_pass2, spr_pass2s);
        else osprites.do_spr_order_shadows(spr_pass2s);
}

// Source: 0x1162
void OCrash::do_crash()
{
    switch (outrun.game_state)
    {
        case GS_INIT_MUSIC:
        case GS_MUSIC:
            end_collision();
            return;

        // Fall through to continue code below
        case GS_ATTRACT:
        case GS_INGAME:
        case GS_BONUS:
            break;

        default:
            // In other modes render crashing ferrari if crash counter is set
            if (crash_counter)
            {
                osprites.do_spr_order_shadows(spr_ferrari);
            }
            // Set Distance Into Screen from crash counter
            spr_ferrari->road_priority = spr_ferrari->counter;
            return;
    }

    // ------------------------------------------------------------------------
    // cont1: Adjust steering
    // ------------------------------------------------------------------------
    int16_t steering_adjust = oinputs.steering_adjust;
    oinputs.steering_adjust = 0;

    if (!oferrari.car_ctrl_active)
    {
        if (spin_control1 == 0)
        {
            uint16_t inc = ((oinitengine.car_increment >> 16) * 31) >> 5;
            oinitengine.car_increment = (oinitengine.car_increment & 0xFFFF) | (inc << 16);
            oferrari.car_inc_old = inc;
        }
        else
        {
            oinputs.steering_adjust = steering_adjust >> 1;
        }
    }

    // ------------------------------------------------------------------------
    // Determine whether to init spin or crash code
    // ------------------------------------------------------------------------
    int16_t spin2_copy = spin_control2;

    // dec_spin2:
    if (spin2_copy != 0)
    {
        spin2_copy -= 2;
        if (spin2_copy < 0)
            spin_control2 = 0;
        else
            spin_switch(spin2_copy);
    }

    // dec_spin1:
    int16_t spin1_copy = spin_control1;

    if (spin1_copy != 0)
    {
        spin1_copy -= 2;
        if (spin1_copy < 0)
            spin_control1 = 0;
        else
            spin_switch(spin1_copy);
    }
    // Not spinning, init crash code
    else
        crash_switch();
}

// Source: 0x1224
void OCrash::spin_switch(const uint16_t ctrl)
{
    crash_counter++;
    crash_z = 0;
    
    switch (ctrl & 3)
    {
        // No Spin - Need to init crash/spin routines
        case 0:
            init_collision();
            break;
        // Init Spin
        case 1:
            do_collision();
            break;
        // Spin In Progress - End Collision Routine
        case 2:
        case 3:
            end_collision();
            break;
    }
}

// Init Crash.
//
// Source: 0x1252
void OCrash::crash_switch()
{
    crash_counter++;
    crash_z = 0;

    switch (crash_state & 7)
    {
        // No Crash. Need to setup crash routines.
        case 0:
            init_collision();
            break;
        // Initial Collision
        case 1:
            if ((crash_type & 3) == 0) do_bump();
            else do_collision();
            break;
        // Flip Car
        case 2:
            do_car_flip();
            break;
        // Horizontally Flip Car, Trigger Smoke Cloud
        case 3:
        case 4:
            trigger_smoke();
            break;
        // Do Girl Pointing Finger Animation/Delay Before Pan
        case 5:
            post_flip_anim();
            break;
        // Pan Camera To Track Centre
        case 6:
            pan_camera();
            break;
        // Camera Repositioned. Prepare For Restart.
        case 7:
            end_collision();
            break;
    }
}

// Init Collision. Used For Spin & Flip
//
// Source: 0x1962
void OCrash::init_collision()
{
    oferrari.car_state = OFerrari::CAR_ANIM_SEQ; // Denote car animation sequence

    // Enable crash sprites
    spr_shadow->control |= OSprites::ENABLE;
    spr_pass1->control  |= OSprites::ENABLE;
    spr_pass2->control  |= OSprites::ENABLE;

    // Disable normal sprites
    oferrari.spr_ferrari->control &= ~OSprites::ENABLE;
    oferrari.spr_shadow->control  &= ~OSprites::ENABLE;
    oferrari.spr_pass1->control   &= ~OSprites::ENABLE;
    oferrari.spr_pass2->control   &= ~OSprites::ENABLE;

    spr_ferrari->x = oferrari.spr_ferrari->x;
    spr_ferrari->y = 221;
    spr_ferrari->counter = 0x1FC;
    spr_ferrari->draw_props = oentry::BOTTOM;

    // Collided with another vechicle
    if (spin_control2)
        init_spin2();
    else if (spin_control1)
        init_spin1();
    // Crash into scenery
    else
    {
        skid_counter = 0;
        uint16_t car_inc = oinitengine.car_increment >> 16;
        if (car_inc < 0x64)
            collide_slow();
        else if (car_inc < 0xC8)
            collide_med();
        else
            collide_fast();
    }
}

// This code also triggers a flip, if the crash_type is set correctly.
// Source: 0x138C
void OCrash::do_collision()
{
    if (olevelobjs.collision_sprite)
    {
        olevelobjs.collision_sprite = 0;
        if (spin_control1 || spin_control2)
        {
            spin_control2 = 0;
            spin_control1 = 0;
            init_collision(); // Init collision with another sprite
            return;
        }

        // Road generator 1
        if (oinitengine.car_x_pos - (oroad.road_width >> 16) >= 0)
        {
            if (slide < 0)
            {
                slide = -slide;
                oinitengine.car_x_pos -= slide;
                osoundint.queue_sound(sound::CRASH2);
            }
        }
        // Road generator 2
        else
        {
            if (slide >= 0)
            {
                slide = -slide;
                oinitengine.car_x_pos -= slide;
                osoundint.queue_sound(sound::CRASH2);
            }
        }
    }
    // 0x13F8
    uint32_t property_table = addr + (frame << 3);
    crash_z = spr_ferrari->counter;
    spr_ferrari->zoom = 0x80;
    spr_ferrari->priority = 0x1FD;
    oinitengine.car_x_pos -= slide;
    spr_ferrari->addr = roms.rom0p->read32(property_table);

    if (roms.rom0p->read8(4 + property_table))
        spr_ferrari->control |= OSprites::HFLIP;
    else
        spr_ferrari->control &= ~OSprites::HFLIP;

    spr_ferrari->pal_src = roms.rom0p->read8(5 + property_table);
    spin_pass_frame = (int8_t) roms.rom0p->read8(6 + property_table);

    if (--spinflipcount2 > 0)
    {
        done(spr_ferrari);
        return;
    }

    spinflipcount2 = crash_spin_count; // Expired: spinflipcount

    if (spinflipcount1)
    {
        frame++;

        // 0x1470
        // Initialize Car Flip
        if (!spin_control1 && !spin_control2 && frame == 2 && crash_type != CRASH_SPIN)
        {
            crash_state = 2; // flip
            addr = outrun.adr.sprite_crash_flip;
            spinflipcount1 = 3; // 3 flips remaining
            spinflipcount2 = crash_spin_count;
            frame = 0;

            // Enable passenger shadows
            spr_pass1s->control |= OSprites::ENABLE;
            spr_pass2s->control |= OSprites::ENABLE;
            done(spr_ferrari);
            return;
        }
        // Do Spin
        else
        {
            if (slide > 0)
                slide -= 2;
            else if (slide < -2)
                slide += 2;

            // End of frame sequence
            if (!roms.rom0p->read8(7 + property_table))
            {
                done(spr_ferrari);
                return;
            }
        }
    }
    // 0x14F4
    frame = 0;
    
    // Last spin
    if (--spinflipcount1 <= 0)
    {
        osoundint.queue_sound(sound::STOP_SLIP);
        if (spin_control2)
        {
            spin_control2++;
        }
        else if (spin_control1)
        {
            spin_control1++;
        }
        // Init smoke
        else
        {
            crash_state = 4; // trigger smoke
            crash_spin_count = 1;
            spr_ferrari->x += slide; // inc ferrari x based on slide value
        }
    }
    else
        crash_spin_count++;

    done(spr_ferrari);
}

// Source: 0x1D0C
void OCrash::end_collision()
{
    // Enable 'normal' Ferrari object
    oferrari.spr_ferrari->control |= OSprites::ENABLE;
    oferrari.spr_shadow->control  |= OSprites::ENABLE;
    oferrari.spr_pass1->control   |= OSprites::ENABLE;
    oferrari.spr_pass2->control   |= OSprites::ENABLE;

    coll_count2 = coll_count1;
    if (!coll_count2)
        coll_count2 = coll_count1 = 1;

    crash_counter = 0;
    crash_state = 0;
    olevelobjs.collision_sprite = 0;
    
    oferrari.spr_ferrari->x = 0;
    oferrari.spr_ferrari->y = 221;
    oferrari.car_ctrl_active = true;
    oferrari.car_state = OFerrari::CAR_NORMAL;
    olevelobjs.spray_counter = 0;
    crash_z = 0;

    if (spin_control1)
        oferrari.car_inc_old = oinitengine.car_increment >> 16;
    else
        oferrari.reset_car();

    spin_control2 = 0;
    spin_control1 = 0;

    spr_ferrari->control &= ~OSprites::ENABLE;
    spr_shadow->control  &= ~OSprites::ENABLE;
    spr_pass1->control   &= ~OSprites::ENABLE;
    spr_pass1s->control  &= ~OSprites::ENABLE;
    spr_pass2->control   &= ~OSprites::ENABLE;
    spr_pass2s->control  &= ~OSprites::ENABLE;

    function_pass1 = &OCrash::do_crash_passengers;
    function_pass2 = &OCrash::do_crash_passengers;
    oinputs.crash_input = 0x10; // Set delay in processing steering
}

// Low Speed Bump - Car Rises in air and sinks
// Source: 0x12BE
void OCrash::do_bump()
{
    oferrari.car_ctrl_active = false;   // Disable user control of car
    spr_ferrari->zoom = 0x80;           // Set Entry Number For Zoom Lookup Table
    spr_ferrari->priority = 0x1FD;
    
    int16_t new_position = (int8_t) roms.rom0.read8(DATA_MOVEMENT + (lookup_index << 3));

    if (new_position)
        crash_z = spr_ferrari->counter;

    spr_ferrari->y = 221 - (new_position >> shift);

    uint32_t frames = addr + (frame << 3);
    spr_ferrari->addr = roms.rom0p->read32(frames);
    
    if (roms.rom0p->read8(frames + 4))
        spr_ferrari->control |= OSprites::HFLIP;
    else
        spr_ferrari->control &= ~OSprites::HFLIP;
    
    spr_ferrari->pal_src = roms.rom0p->read8(frames + 5);
    spin_pass_frame = (int8_t) roms.rom0p->read8(frames + 6);

    if (++lookup_index >= 0x10)
    {
        addr += (frame_restore << 3);
        spr_ferrari->addr = roms.rom0p->read32(addr);
        spin_pass_frame = (int8_t) roms.rom0p->read8(addr + 6);
        crash_state = 4;      // Trigger smoke cloud
        crash_spin_count = 1; // Denote Crash
    }

    done(spr_ferrari);
}

// Source: 0x1562
void OCrash::do_car_flip()
{
    // Do this if during the flip, the car has recollided with a new sprite + slow crash (similar to spin_collide)
    if (olevelobjs.collision_sprite && crash_speed == 1)
    {
        uint16_t car_inc16 = oinitengine.car_increment >> 16;

        // Road generator 1
        if (oinitengine.car_x_pos - (oroad.road_width >> 16) >= 0)
        {
            // swap_slide_dir2
            if (slide < 0)
            {
                slide = -slide;
                oinitengine.car_increment = ((car_inc16 >> 1) << 16) | (oinitengine.car_increment & 0xFFFF);
                osoundint.queue_sound(sound::CRASH2);
                if (oinitengine.car_increment >> 16 > 0x14)
                {
                    int16_t z = spr_ferrari->counter > 0x1FD ? 0x1FD : spr_ferrari->counter; // d3
                    int16_t x_adjust = (0x50 * z) >> 9; // d1
                    if (slide < 0) x_adjust = -x_adjust;
                    oinitengine.car_x_pos -= x_adjust;
                }
            }
            // 0x15F6
            else
                slide += (slide >> 3);
        }
        // Road generator 2
        else
        {
            // swap_slide_dir2:
            if (slide >= 0)
            {
                slide = -slide;
                oinitengine.car_increment = ((car_inc16 >> 1) << 16) | (oinitengine.car_increment & 0xFFFF);
                osoundint.queue_sound(sound::CRASH2);
                if (oinitengine.car_increment >> 16 > 0x14)
                {
                    int16_t z = spr_ferrari->counter > 0x1FD ? 0x1FD : spr_ferrari->counter; // d3
                    int16_t x_adjust = (0x50 * z) >> 9; // d1
                    if (slide < 0) x_adjust = -x_adjust;
                    oinitengine.car_x_pos -= x_adjust;
                }
            }
            // 0x15F6
            else
                slide += (slide >> 3);
        }
    }
    
    // flip_cont
    olevelobjs.collision_sprite = 0; // Moved this for clarity
    uint32_t frames = addr + (frame << 3);
    spr_ferrari->addr = roms.rom0p->read32(frames);

    // ------------------------------------------------------------------------
    // Fast Crash: Car Heads towards camera in sky, before vanishing (0x161E)
    // ------------------------------------------------------------------------
    if (crash_speed == 0)
    {
        spr_shadow->control &= ~OSprites::ENABLE; // Disable Shadow
        spr_ferrari->counter += crash_zinc;       // Increment Crash Z
        if (spr_ferrari->counter > 0x3FF)
        {
            spr_ferrari->zoom = 0;
            spr_ferrari->counter = 0;
            init_finger(frames);
            done(spr_ferrari);
            return;
        }
        else
            crash_zinc++;
    }
    // ------------------------------------------------------------------------
    // Slow Crash (0x1648 flip_slower)
    // ------------------------------------------------------------------------
    else
    {
        spr_ferrari->counter -= crash_zinc;       // Decrement Crash Z
        if (crash_zinc > 2)
            crash_zinc--;
    }

    // set_crash_z_inc
    // Note that we've set crash_zinc previously now for clarity

    // use ferrari_crash_z to set priority
    spr_ferrari->priority = spr_ferrari->counter > 0x1FD ? 0x1FD : spr_ferrari->counter;

    int16_t x_diff = (slide * spr_ferrari->priority) >> 9;
    oinitengine.car_x_pos -= x_diff;

    int16_t passenger_frame = (int8_t) roms.rom0p->read8(6 + frames);

    // Start of sequence
    if (passenger_frame == 0)
    {
        slide >>= 1;
        osoundint.queue_sound(sound::CRASH2);
    }

    // Set Z during lower frames
    if (passenger_frame <= 0x10 && spr_ferrari->counter <= 0x1FE)
        crash_z = spr_ferrari->counter;

    // 0x16CC
    passenger_frame = (passenger_frame * spr_ferrari->priority) >> 9;

    // Set Ferrari Y
    int16_t y = -(oroad.road_y[oroad.road_p0 + spr_ferrari->priority] >> 4) + 223;
    y -= passenger_frame;
    spr_ferrari->y = y;

    // Set Ferrari Zoom from Z
    spr_ferrari->zoom = (spr_ferrari->counter >> 2);
    if (spr_ferrari->zoom < 0x40) spr_ferrari->zoom = 0x40;

    // Set Ferrari H-Flip
    if (crash_side) 
        spr_ferrari->control |= OSprites::HFLIP;
    else 
        spr_ferrari->control &= ~OSprites::HFLIP;

    spr_ferrari->pal_src = roms.rom0p->read8(4 + frames);

    if (--spinflipcount2 > 0)
    {
        done(spr_ferrari);
        return;
    }

    spinflipcount2 = crash_spin_count;

    // 0x1736
    // Advance to next frame in sequence
    if (spinflipcount1)
    {
        frame++;
        // End of frame sequence
        if ((roms.rom0p->read8(7 + frames) & BIT_7) == 0)
        {
            done(spr_ferrari);
            return;
        }
    }

    frame = 0;
    
    if (--spinflipcount1 > 0)
    {
        crash_spin_count++;
    }
    else
    {
        init_finger(frames);
    }

    done(spr_ferrari);
}

// Init Delay/Girl Pointing Finger
// Source: 0x175C
void OCrash::init_finger(uint32_t frames)
{
    crash_spin_count = 1;           // Denote Crash has taken place
    
    // Do Delay whilst girl points finger
    if (crash_type == CRASH_FLIP)
    {
        oferrari.wheel_state = OFerrari::WHEELS_ON;
        oferrari.car_state   = OFerrari::CAR_NORMAL;
        slide = 0;
        addr += frames;
        crash_delay = 30;
        crash_state = 5; 
    }
    // Slide Car and Trigger Smoke Cloud
    else
    {
        crash_state = 3;
        frame = 0;
        addr = outrun.adr.sprite_crash_man1;
        crash_spin_count = 4;   // Denote third flip
        spinflipcount2   = 4;
    }
}

// Post Crash: Slide Car Slightly, then trigger smoke
// Source: 0x17D2
void OCrash::trigger_smoke()
{
    crash_z = spr_ferrari->counter;
    int16_t slide_copy = slide;

    if (slide < 0)
        slide++;
    else if (slide > 0)
        slide--;

    // Slide Car
    oinitengine.car_x_pos -= slide_copy;

    spr_ferrari->addr = roms.rom0p->read32(addr);

    // Set Ferrari H-Flip
    if (roms.rom0p->read8(4 + addr))
        spr_ferrari->control |= OSprites::HFLIP;
    else 
        spr_ferrari->control &= ~OSprites::HFLIP;

    spr_ferrari->pal_src =     roms.rom0p->read8(5 + addr);
    spin_pass_frame = (int8_t) roms.rom0p->read8(6 + addr);

    // Slow Car
    oinitengine.car_increment = 
        (oinitengine.car_increment - ((oinitengine.car_increment >> 2) & 0xFFFF0000)) 
        | (oinitengine.car_increment & 0xFFFF);

    // Car stationary
    if (oinitengine.car_increment >> 16 <= 0)
    {
        oinitengine.car_increment = 0;
        oferrari.wheel_state = OFerrari::WHEELS_ON;
        oferrari.car_state   = OFerrari::CAR_NORMAL;
        slide = 0;
        crash_delay = 30;
        crash_state = 5; // post crash animation delay state
    }

    done(spr_ferrari);
}

// Source: 0x1870
void OCrash::post_flip_anim()
{
    oferrari.car_ctrl_active = false;  // Car and road updates disabled
    if (--crash_delay > 0)
    {
        done(spr_ferrari);
        return;
    }

    oferrari.car_ctrl_active = true;
    crash_state = 6; // Denote pan camera to track centre
    
    int16_t road_width = oroad.road_width >> 16;
    int16_t car_x_pos  = oinitengine.car_x_pos;
    camera_xinc = 8;
    
    // Double Road
    if (road_width >= 0xD7)
    {
        if (car_x_pos < 0)
            road_width = -road_width;
    }
    else
    {
        road_width = 0;
    }

    camera_x_target = road_width;

    // 1/ Car on road generator 1 (1 road enabled)
    // 2/ Car on road generator 1 (2 roads enabled)
    if (road_width < car_x_pos)
    {
        camera_xinc += (car_x_pos - road_width) >> 6;
        camera_xinc = -camera_xinc;
    }
    else
    {
        camera_xinc += (road_width - car_x_pos) >> 6;
    }

    done(spr_ferrari);
}

// Pan Camera Back To Centre After Flip
// Source: 0x18EC
void OCrash::pan_camera()
{
    oferrari.car_ctrl_active = true;

    oinitengine.car_x_pos += camera_xinc;

    int16_t x_diff = (oferrari.car_x_diff * spr_ferrari->counter) >> 9;
    spr_ferrari->x += x_diff;

    // Pan Right
    if (camera_xinc >= 0)
    {
        if (camera_x_target <= oinitengine.car_x_pos)
            crash_state = 7; // Denote camera position and ready for restart
    }
    // Pan Left
    else
    {
        if (camera_x_target >= oinitengine.car_x_pos)
            crash_state = 7;
    }

    done(spr_ferrari);
}

// Source: 0x1C7E
void OCrash::init_spin1()
{
    osoundint.queue_sound(sound::INIT_SLIP);
    uint16_t car_inc = oinitengine.car_increment >> 16;
    uint16_t spins = 1;
    if (car_inc > 0xB4)
        spins += outils::random() & 1;

    spinflipcount1 = spins;
    crash_spin_count = 2;
    spinflipcount2 = 2;

    slide = ((spins + 1) << 2) + (car_inc > 0xFF) ? 0xFF >> 3 : car_inc >> 3;

    if (skid_counter_bak < 0)
        addr = outrun.adr.sprite_crash_spin1;
    else
    {
        addr = outrun.adr.sprite_crash_spin1;
        slide = -slide;
    }
    
    spin_control1++;
    frame = 0;
    skid_counter = 0;
    spr_ferrari->road_priority = spr_ferrari->counter;
}

// Source: 0x1C10
void OCrash::init_spin2()
{
    osoundint.queue_sound(sound::INIT_SLIP);
    uint16_t car_inc = oinitengine.car_increment >> 16;
    spinflipcount1 = 1;
    crash_spin_count = 2;
    spinflipcount2 = 8;

    slide = (car_inc > 0xFF) ? 0xFF >> 3 : car_inc >> 3;

    if (oinitengine.road_type != OInitEngine::ROAD_RIGHT)
    {
        addr = outrun.adr.sprite_crash_spin1;  
    }
    else
    {
        addr = outrun.adr.sprite_crash_spin1;
        slide = -slide;
    }

    spin_control2++;
    frame = 0;
    skid_counter = 0;
    spr_ferrari->road_priority = spr_ferrari->counter;
}

// Collision: Slow 
// Rebound and bounce car in air
// Source: 0x19EE
void OCrash::collide_slow()
{
    osoundint.queue_sound(sound::REBOUND);
    
    // Setup shift value for car bump, based on current speed, which ultimately determines how much car rises in air
    uint16_t car_inc = oinitengine.car_increment >> 16;

    if (car_inc <= 0x28)
        shift = 6;
    else if (car_inc <= 0x46)
        shift = 5;
    else
        shift = 4;

    lookup_index = 0;

    // Calculate change in road y, so we can determine incline frame for ferrari
    int16_t y = oroad.road_y[oroad.road_p0 + (0x3E0 / 2)] - oroad.road_y[oroad.road_p0 + (0x3F0 / 2)];
    frame_restore = 0;
    if (y >= 0x12) frame_restore++;
    if (y >= 0x13) frame_restore++;
    
    // Right Hand Side: Increment Frame Entry By 3
    if (oinitengine.car_x_pos < 0)
        addr = outrun.adr.sprite_bump_data2;
    else
        addr = outrun.adr.sprite_bump_data1;

    crash_type = CRASH_BUMP; // low speed bump
    oinitengine.car_increment &= 0xFFFF;

    // set_collision:
    frame = 0;
    crash_state = 1; // collision with object
    spr_ferrari->road_priority = spr_ferrari->counter;
}

// Collision: Medium
// Spin car
// Source: 0x1A98
void OCrash::collide_med()
{
    osoundint.queue_sound(sound::INIT_SLIP);
    
    // Set number of spins based on car speed
    uint16_t car_inc = oinitengine.car_increment >> 16;    
    spinflipcount1 = car_inc <= 0x96 ? 1 : 2;
    spinflipcount2 = crash_spin_count = 2;
    slide = ((spinflipcount1 + 1) << 2) + ((car_inc > 0xFF ? 0xFF : car_inc) >> 3);

    // Right Hand Side: Increment Frame Entry By 3
    if (oinitengine.car_x_pos < 0)
    {
        addr = outrun.adr.sprite_crash_spin1;
        slide = -slide;
    }
    else
        addr = outrun.adr.sprite_crash_spin1;

    crash_type = CRASH_SPIN;

    // set_collision:
    frame = 0;
    crash_state = 1; // collision with object
    spr_ferrari->road_priority = spr_ferrari->counter;
}

// Collision: Fast
// Spin, Then Flip Car
//
// Source: 0x1B12
void OCrash::collide_fast()
{
    osoundint.queue_sound(sound::CRASH1);

    uint16_t car_inc = oinitengine.car_increment >> 16;
    if (car_inc > 0xFA)
    {
        crash_zinc = 1;
        crash_speed = 0;
    }
    else
    {
        crash_zinc = 0x10;
        crash_speed = 1;
    }

    spinflipcount1 = 1;
    spinflipcount2 = crash_spin_count = 2;
    
    slide = (car_inc > 0xFF ? 0xFF : car_inc) >> 2;
    slide += (slide >> 1);

    if (oinitengine.road_type != OInitEngine::ROAD_STRAIGHT)
    {
        int16_t d2 = (0x78 - (oinitengine.road_curve <= 0x78 ? oinitengine.road_curve : 0x78)) >> 1;

        // collide_fast_curve:
        slide += d2;
        if (oinitengine.road_type == OInitEngine::ROAD_RIGHT)
            slide = -slide;
    }
    else
    {
        if (oinitengine.car_x_pos < 0) slide = -slide; // rhs
    }
    
    // set_fast_slide:
    if (slide > 0x78) 
        slide = 0x78;

    if (oinitengine.car_x_pos < 0)
    {
        addr = outrun.adr.sprite_crash_spin2;
        crash_side = 0; // RHS
    }
    else
    {
        addr = outrun.adr.sprite_crash_spin1;
        crash_side = 1; // LHS
    }

    crash_type = CRASH_FLIP; // Flip

    // set_collision:
    frame = 0;
    crash_state = 1; // collision with object
    spr_ferrari->road_priority = spr_ferrari->counter;
}

// Source: 0x1556
void OCrash::done(oentry* sprite)
{
    osprites.map_palette(sprite);
    osprites.do_spr_order_shadows(sprite);
    sprite->road_priority = sprite->counter;
}

// ------------------------------------------------------------------------------------------------
//                                      SHADOW CRASH ROUTINES
// ------------------------------------------------------------------------------------------------

// Render Shadow
//
// Disabled during fast car flip, when car rapidly heading towards screen
//
// Source: 0x1DF2
void OCrash::do_shadow(oentry* src_sprite, oentry* dst_sprite)
{
    uint8_t shadow_shift;

    // Ferrari Shadow
    if (src_sprite == spr_ferrari)
    {
        dst_sprite->draw_props = oentry::BOTTOM;
        shadow_shift = 1;
    }
    else
    {
        shadow_shift = 3;
    }

    dst_sprite->x = src_sprite->x;
    dst_sprite->road_priority = src_sprite->road_priority;

    // Get Z from source sprite (stored in counter)
    uint16_t counter = (src_sprite->counter) >> shadow_shift;
    counter = counter - (counter >> 2);
    dst_sprite->zoom = (uint8_t) counter;

    // Set shadow y
    uint16_t offset = src_sprite->counter > 0x1FF ? 0x1FF : src_sprite->counter;
    dst_sprite->y = -(oroad.road_y[oroad.road_p0 + offset] >> 4) + 223;

    osprites.do_spr_order_shadows(dst_sprite);
}

// ------------------------------------------------------------------------------------------------
//                                     PASSENGER CRASH ROUTINES
// ------------------------------------------------------------------------------------------------

// Flips & Spins Only
//
// Process passengers during crash scenario.
//
// Source: 0x1E66
void OCrash::do_crash_passengers(oentry* sprite)
{
    // --------------------------------------------------------------------------------------------
    // Flip car
    // --------------------------------------------------------------------------------------------
    if (crash_state == 2)
    {
        // Update pointer to functions
        if (sprite == spr_pass1) 
            function_pass1 = &OCrash::flip_start;
        else if (sprite == spr_pass2) 
            function_pass2 = &OCrash::flip_start;

        // Crash Passenger Flip
        crash_pass_flip(sprite);
        return;
    }

    // --------------------------------------------------------------------------------------------
    // Non-Flip
    // --------------------------------------------------------------------------------------------
    if (crash_state < 5)
        crash_pass1(sprite);
    else
        crash_pass2(sprite);

    osprites.map_palette(sprite);
    osprites.do_spr_order_shadows(sprite);
}

// Position Passenger Sprites During Crash (But Not Flip)
//
// - Process passenger sprites in crash scenario
// - Called separately for man and girl
//
// Source: 0x1EA6
void OCrash::crash_pass1(oentry* sprite)
{
    uint32_t frames = (sprite == spr_pass1 ? outrun.adr.sprite_crash_man1 : outrun.adr.sprite_crash_girl1) + (spin_pass_frame << 3);
    
    sprite->addr    = roms.rom0p->read32(frames);
    uint8_t props   = roms.rom0p->read8(4 + frames);
    sprite->pal_src = roms.rom0p->read8(5 + frames);
    sprite->x       = spr_ferrari->x + (int8_t) roms.rom0p->read8(6 + frames);
    sprite->y       = spr_ferrari->y + (int8_t) roms.rom0p->read8(7 + frames);

    // Check H-Flip
    if (props & BIT_7)
        sprite->control |= OSprites::HFLIP;
    else
        sprite->control &= ~OSprites::HFLIP;

    // Test whether we should set priority higher (unused on passenger sprites I think)
    if (props & BIT_0)
        sprite->priority = sprite->road_priority = 0x1FE;
    else
        sprite->priority = sprite->road_priority = 0x1FD;

    sprite->zoom = 0x7E;
}

// Passenger animations following the crash sequence i.e. when car is stationary
//
// 3 = hands battering,
// 2 = man scratches head, girl taps car
// 1 = man scratch head & girl points
// 0 = man subdued & girl points
//
// - Process passenger sprites in crash scenario
// - Called separately for man and girl
// - Selects which passenger animation to play
//
// Source: 0x1F26
void OCrash::crash_pass2(oentry* sprite)
{
    uint32_t frames = (sprite == spr_pass1 ? outrun.adr.sprite_crash_man2 : outrun.adr.sprite_crash_girl2);

    // Use coll_count2 to select one of the three animations that can be played
    // Use crash_delay to toggle between two distinct frames
    frames += ((coll_count2 & 3) << 4) + (crash_delay & 8);
    
    sprite->addr    = roms.rom0p->read32(frames);
    uint8_t props   = roms.rom0p->read8(4 + frames);
    sprite->pal_src = roms.rom0p->read8(5 + frames);
    sprite->x       = spr_ferrari->x + (int8_t) roms.rom0p->read8(6 + frames);
    sprite->y       = spr_ferrari->y + (int8_t) roms.rom0p->read8(7 + frames);

    // Check H-Flip
    if (props & BIT_7)
        sprite->control |= OSprites::HFLIP;
    else
        sprite->control &= ~OSprites::HFLIP;

    // Test whether we should set priority higher (unused on passenger sprites I think)
    if (props & BIT_0)
        sprite->priority = sprite->road_priority = 0x1FF;
    else
        sprite->priority = sprite->road_priority = 0x1FE;

    sprite->zoom = 0x7E;

    // Man
    if (sprite == spr_pass1)
    {
        const int8_t XY_OFF[] =
        {
            -0xC, -0x1E, 
            0x2,  -0x1B, 
            0x4,  -0x1A, 
            0x5,  -0x1E,
            0x11, -0x1B,
            0x0,  -0x1A,
            -0x1, -0x1B,
            -0xC, -0x1C,
            -0xE, -0x1B,
            -0xE, -0x1C,
            -0xE, -0x1D,
            -0xC, -0x1B,
            -0xC, -0x1C,
            -0xC, -0x1D,
        };

        sprite->x += XY_OFF[spin_pass_frame << 1];
        sprite->y += XY_OFF[(spin_pass_frame << 1) + 1];
    }
    // Woman
    else
    {
        const int8_t XY_OFF[] =
        {
            0xA,   -0x1A,
            0x0,   -0x1B,
            -0xF,  -0x1B,
            -0x15, -0x1B,
            -0x2,  -0x1E,
            0x7,   -0x1A,
            0x13,  -0x1D,
            0x9,   -0x1B,
            0x3,   -0x1B,
            0x3,   -0x1C,
            0x3,   -0x1D,
            0x7,   -0x1B,
            0x7,   -0x1C,
            0x7,   -0x1D,
        };

        sprite->x += XY_OFF[spin_pass_frame << 1];
        sprite->y += XY_OFF[(spin_pass_frame << 1) + 1];
    }
}

// Handle passenger animation sequence during car flip
//
// 3 main stages:
// 1/ Flip passengers out of car
// 2/ Passengers sit up on road after crash
// 3/ Passengers turn head and look at car (only if camera pan)
//
// Source: 0x1FDE

void OCrash::crash_pass_flip(oentry* sprite)
{
    // Some of these variable names really need refactoring
    sprite->reload        = 0;                      // clear passenger flip control
    sprite->xw1           = 0;
    sprite->x             = spr_ferrari->x;
    sprite->traffic_speed = crash_spin_count;
    sprite->counter       = 0x1FE;                  // sprite zoom

    // Set address of animation sequence based on whether male/female
    sprite->z = sprite == spr_pass1 ? outrun.adr.sprite_crash_flip_m1 : outrun.adr.sprite_crash_flip_g1;

    flip_start(sprite);
}

// Source: 0x201A
void OCrash::flip_start(oentry* sprite)
{
    if (outrun.game_state != GS_ATTRACT && outrun.game_state != GS_INGAME)
    {
        osprites.do_spr_order_shadows(sprite);
        done(sprite);
        return;
    }

    switch (sprite->reload & 3) // check passenger flip control
    {
        // Flip passengers out of car
        case 0:
            pass_flip(sprite);
            break;

        // Passengers sit up on road after crash
        case 1:
            pass_situp(sprite);
            break;

        // Passengers turn head and look at car (only if camera pan)
        case 2:
        case 3:
            pass_turnhead(sprite);
            break;
    }
}

// Flip passengers out of car
// Source: 0x2066
void OCrash::pass_flip(oentry* sprite)
{
    // Fast crash
    if (crash_speed == 0)
    {
        sprite->counter += (crash_zinc << 2);

        if (sprite->counter > 0x3FF)
        {
            sprite->reload = 1; // // Passenger Control: Passengers sit up on road after crash

            // Disable sprite and shadow
            sprite->control &= ~OSprites::ENABLE;
            if (sprite == spr_pass1)
                spr_pass1s->control &= ~OSprites::ENABLE;
            else
                spr_pass2s->control &= ~OSprites::ENABLE;
            return;
        }
    }
    // Slow crash
    else
    {
        int16_t zinc = crash_zinc >> 2;
    
        // Adjust the z position of the female more than the man
        if (sprite == spr_pass2)
        {
            zinc += (zinc >> 1);
        }

        sprite->counter -= zinc;
    }

    // set_z_lookup
    int16_t zoom = sprite->counter >> 2;
    if (zoom < 0x40) zoom = 0x40;
    sprite->zoom = (uint8_t) zoom;

    uint32_t frames = sprite->z + (sprite->xw1 << 3);
    sprite->addr = roms.rom0p->read32(frames);

    uint16_t offset = sprite->counter > 0x1FF ? 0x1FF : sprite->counter;
    int16_t y_change = (((int8_t) roms.rom0p->read8(6 + frames)) * offset) >> 9; // d1

    sprite->y = -(oroad.road_y[oroad.road_p0 + offset] >> 4) + 223;
    sprite->y -= y_change;

    // 2138

    sprite->priority = offset;
    if (crash_side) 
        sprite->control |= OSprites::HFLIP;
    else 
        sprite->control &= ~OSprites::HFLIP;

    sprite->pal_src = roms.rom0p->read8(4 + frames);
    
    // Decrement spin count
    // Increment frame of passengers for first spins
    if (--sprite->traffic_speed <= 0)
    {
        sprite->traffic_speed = crash_spin_count;
        sprite->xw1++; // Increase passenger frame

        // End of animation sequence. Progress to next sequnce of animations.
        if (roms.rom0p->read8(7 + frames) & BIT_7)
        {
            sprite->reload = 1; // Passenger Control: Passengers sit up on road after crash
            sprite->xw1    = 0; // Reset passenger frame
            
            // Update address of animation sequence to be used
            sprite->z      = sprite == spr_pass1 ? outrun.adr.sprite_crash_flip_m2 : outrun.adr.sprite_crash_flip_g2;
            frames         = sprite->z;

            // Set Frame Delay for this animation sequence from lower bytes
            sprite->traffic_speed = roms.rom0p->read8(7 + frames) & 0x7F;

            done(sprite);
            return;
        }
    }

    // set_passenger_x
    sprite->x =  spr_ferrari->x;
    sprite->x += ((int8_t) roms.rom0p->read8(5 + frames));
    done(sprite);
}

// Passengers sit up on road after crash
// Source: 0x205A
void OCrash::pass_situp(oentry* sprite)
{
    // Update passenger x position
    int16_t x_diff = (oferrari.car_x_diff * sprite->counter) >> 9;
    sprite->x += x_diff;

    uint32_t frames = sprite->z + (sprite->xw1 << 3);
    sprite->addr    = roms.rom0p->read32(frames);
    sprite->pal_src = roms.rom0p->read8(4 + frames);

    // Decrement frame delay counter
    if (--sprite->traffic_speed <= 0)
    {
        sprite->traffic_speed = roms.rom0p->read8(0xF + frames) & 0x7F;

        // End of animation sequence. Progress to next sequnce of animations.
        if (roms.rom0p->read8(7 + frames) & BIT_7)
        {
            sprite->reload = 2; // Passenger Control: Passengers turn head and look at car
        }
        else
        {
            sprite->xw1++; // Increase passenger frame

            // If camera pan: Make passengers turn heads!
            if (crash_state == 6)
                    sprite->reload = 2;
        }
    }
    done(sprite);
}

// Passengers turn head and look at car (only if camera pan)
// Source: 0x222C
void OCrash::pass_turnhead(oentry* sprite)
{
    // Update passenger x position
    int16_t x_diff = (oferrari.car_x_diff * sprite->counter) >> 9;
    sprite->x += x_diff;

    uint32_t frames = sprite->z + (sprite->xw1 << 3);
    sprite->addr    = roms.rom0p->read32(frames);
    sprite->pal_src = roms.rom0p->read8(4 + frames);

    // End of animation sequence.
    if (roms.rom0p->read8(7 + frames) & BIT_7)
    {
        done(sprite);
        return;
    }

    // Decrement frame delay counter
    if (--sprite->traffic_speed <= 0)
    {
        sprite->traffic_speed = roms.rom0p->read8(0xF + frames) & 0x7F;
        sprite->xw1++; // Increase passenger frame
    }

    done(sprite);
}