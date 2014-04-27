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

#include "engine/oanimseq.hpp"
#include "engine/oattractai.hpp"
#include "engine/obonus.hpp"
#include "engine/ocrash.hpp"
#include "engine/ohud.hpp"
#include "engine/oinputs.hpp"
#include "engine/olevelobjs.hpp"
#include "engine/ooutputs.hpp"
#include "engine/ostats.hpp"
#include "engine/outils.hpp"
#include "engine/oferrari.hpp"

OFerrari oferrari;

OFerrari::OFerrari(void)
{
}

OFerrari::~OFerrari(void)
{
}

void OFerrari::init(oentry *f, oentry *p1, oentry *p2, oentry *s)
{
    state       = FERRARI_SEQ1;
    spr_ferrari = f;
    spr_pass1   = p1;
    spr_pass2   = p2;
    spr_shadow  = s;

    spr_ferrari->control |= OSprites::ENABLE;
    spr_pass1->control   |= OSprites::ENABLE;
    spr_pass2->control   |= OSprites::ENABLE;
    spr_shadow->control  |= OSprites::ENABLE;

    state             = 0;
    counter           = 0;
    steering_old      = 0;
    road_width_old    = 0;
    car_state         = CAR_NORMAL;
    auto_brake        = false;
    torque_index      = 0;
    torque            = 0;
    revs              = 0;
    rev_shift         = 0;
    wheel_state       = WHEELS_ON;
    wheel_traction    = TRACTION_ON;
    is_slipping       = 0;
    slip_sound        = 0;
    car_inc_old       = 0;
    car_x_diff        = 0;
    rev_stop_flag     = 0;
    revs_post_stop    = 0;
    acc_post_stop     = 0;
    rev_pitch1        = 0;
    rev_pitch2        = 0;
    sprite_ai_counter = 0;
    sprite_ai_curve   = 0;
    sprite_ai_x       = 0;
    sprite_ai_steer   = 0;
    sprite_car_x_bak  = 0;
    sprite_wheel_state= 0;
    sprite_slip_copy  = 0;
    wheel_pal         = 0;
    sprite_pass_y     = 0;
    wheel_frame_reset = 0;
    wheel_counter     = 0;
    
    road_width_old    = 0;
    accel_value       = 0;
    accel_value_bak   = 0;
    brake_value       = 0;
    gear_value        = false;
    gear_bak          = false;
    brake_subtract    = 0;
    gear_counter      = 0;
    rev_adjust        = 0;
    gear_smoke        = 0;
    gfx_smoke         = 0;
    cornering         = 0;
    cornering_old     = 0;
    car_ctrl_active   = true;
}

// Reset all values relating to car speed, revs etc.
// Source: 0x61F2
void OFerrari::reset_car()
{
    rev_shift            = 1;  // Set normal rev shift value
    ocrash.spin_control2 = 0;
    revs                 = 0;
    oinitengine.car_increment = 0;
    gear_value           = 0;
    gear_bak             = 0;
    rev_adjust           = 0;
    car_inc_old          = 0;
    torque               = 0x1000;
    torque_index         = 0x1F;
    rev_stop_flag        = 0;
    oinitengine.ingame_engine = false;
    oinitengine.ingame_counter = 0x1E; // Set ingame counter (time until we hand control to user)
    slip_sound           = sound::STOP_SLIP;
    acc_adjust1          = 
    acc_adjust2          = 
    acc_adjust3          = 0;
    brake_adjust1        = 
    brake_adjust2        = 
    brake_adjust3        = 0;
    auto_brake           = false;
    counter              = 0;
    is_slipping          = 0;    // Denote not slipping/skidding
}

void OFerrari::tick()
{
    switch (state)
    {
        case FERRARI_SEQ1:
            oanimseq.ferrari_seq();
            oanimseq.anim_seq_intro(&oanimseq.anim_pass1);
            oanimseq.anim_seq_intro(&oanimseq.anim_pass2);
            break;

        case FERRARI_SEQ2:
            oanimseq.anim_seq_intro(&oanimseq.anim_ferrari);
            oanimseq.anim_seq_intro(&oanimseq.anim_pass1);
            oanimseq.anim_seq_intro(&oanimseq.anim_pass2);
            break;

        case FERRARI_INIT:
            if (spr_ferrari->control & OSprites::ENABLE) 
                if (outrun.tick_frame)
                    init_ingame();
            break;

        case FERRARI_LOGIC:
            if (spr_ferrari->control & OSprites::ENABLE) 
            {
                if (outrun.tick_frame)
                    logic();
                else
                    draw_sprite(spr_ferrari);
            }
            if (spr_pass1->control & OSprites::ENABLE) 
            {
                if (outrun.tick_frame)
                    set_passenger_sprite(spr_pass1);
                else
                    draw_sprite(spr_pass1);
            }

            if (spr_pass2->control & OSprites::ENABLE)
            {
                if (outrun.tick_frame)
                    set_passenger_sprite(spr_pass2);
                else
                    draw_sprite(spr_pass2);
            }
            break;

        // Ferrari End Sequence Logic
        case FERRARI_END_SEQ:
                oanimseq.tick_end_seq();
            break;
    }
}

// Initalize Ferrari Start Sequence
//
// Source: 6036
//
// Note the remainder of this block is handled in oanimseq.ferrari_seq
void OFerrari::init_ingame()
{
    // turn_off:
    car_state = CAR_NORMAL;
    state = FERRARI_LOGIC;
    spr_ferrari->reload = 0;
    spr_ferrari->counter = 0;
    sprite_ai_counter = 0;
    sprite_ai_curve = 0;
    sprite_ai_x = 0;
    sprite_ai_steer = 0;
    sprite_car_x_bak = 0;
    sprite_wheel_state = 0;

    // Passengers
    // move.l  #SetPassengerSprite,$42(a5)
    spr_pass1->reload = 0;
    spr_pass1->counter = 0;
    spr_pass1->xw1 = 0;
    // move.l  #SetPassengerSprite,$82(a5)
    spr_pass2->reload = 0;
    spr_pass2->counter = 0;
    spr_pass2->xw1 = 0;
}

// Source: 9C84
void OFerrari::logic()
{
    switch (obonus.bonus_control)
    {
        // Not Bonus Mode
        case OBonus::BONUS_DISABLE:
            ferrari_normal();
            break;

        case OBonus::BONUS_INIT:
            rev_shift = 2;          // Set Double Rev Shift Value
            obonus.bonus_control = OBonus::BONUS_TICK;

        case OBonus::BONUS_TICK:
            oattractai.check_road_bonus();
            oattractai.set_steering_bonus();

            // Accelerate Car
            if (oinitengine.rd_split_state == 0 || (oroad.road_pos >> 16) <= 0x163)
            {
                oinputs.acc_adjust = 0xFF;
                oinputs.brake_adjust = 0;
                setup_ferrari_bonus_sprite();
                return;
            }
            else // init_end_anim
            {
                rev_shift = 1;
                obonus.bonus_control = OBonus::BONUS_SEQ0;
                // note fall through!
            }
            
            case OBonus::BONUS_SEQ0:
                if ((oroad.road_pos >> 16) < 0x18E)
                {
                    init_end_seq();
                    return;
                }
                obonus.bonus_control = OBonus::BONUS_SEQ1;
                // fall through
            
            case OBonus::BONUS_SEQ1:
                if ((oroad.road_pos >> 16) < 0x18F)
                {
                    init_end_seq();
                    return;
                }
                obonus.bonus_control = OBonus::BONUS_SEQ2;

            case OBonus::BONUS_SEQ2:
                if ((oroad.road_pos >> 16) < 0x190)
                {
                    init_end_seq();
                    return;
                }
                obonus.bonus_control = OBonus::BONUS_SEQ3;
            
            case OBonus::BONUS_SEQ3:
                if ((oroad.road_pos >> 16) < 0x191)
                {
                    init_end_seq();
                    return;
                }
                else
                {
                    oferrari.car_ctrl_active = false; // -1
                    oinitengine.car_increment = 0;
                    obonus.bonus_control = OBonus::BONUS_END;
                }

            case OBonus::BONUS_END:
                oinputs.acc_adjust = 0;
                oinputs.brake_adjust = 0xFF;
                do_end_seq();
                break;

        /*default:
            std::cout << "Need to finish OFerrari:logic()" << std::endl;
            break;*/
    }
}

// Ferrari - Normal Game Logic (Non-Bonus Mode Code)
//
// Source: 0x9CB4
void OFerrari::ferrari_normal()
{
    if (FORCE_AI && outrun.game_state == GS_INGAME)
    {
        oattractai.tick_ai_enhanced();
        setup_ferrari_sprite();
        return;
    }

    switch (outrun.game_state)
    {    
        // Attract Mode: Process AI Code and fall through
        case GS_INIT:
        case GS_ATTRACT:
            if (config.engine.new_attract)
                oattractai.tick_ai_enhanced();
            else
                oattractai.tick_ai();
            setup_ferrari_sprite();
            break;

        case GS_INIT_BEST1:
        case GS_BEST1:
        case GS_INIT_LOGO:
        case GS_LOGO:
        case GS_INIT_GAME:
        case GS_INIT_GAMEOVER:
        case GS_GAMEOVER:
            oinputs.brake_adjust = 0;
        case GS_START1:
        case GS_START2:
        case GS_START3:
            oinputs.steering_adjust = 0;
        case GS_INGAME:
        case GS_INIT_BONUS:
        case GS_BONUS:
        case GS_INIT_MAP:
        case GS_MAP:
            setup_ferrari_sprite();
            break;

        case GS_INIT_MUSIC:
        case GS_MUSIC:
            return;

        /*default:
            std::cout << "Need to finish OFerrari:ferrari_normal()" << std::endl;
            break;*/
    }
}

// Setup Ferrari Sprite Object
//
// Source: 0x9D30
void OFerrari::setup_ferrari_sprite()
{
    spr_ferrari->y = 221; // Set Default Ferrari Y
    
    // Test Collision With Other Sprite Object
    if (olevelobjs.collision_sprite)
    {
        if (ocrash.coll_count1 == ocrash.coll_count2)
        {
            ocrash.coll_count1++;
            olevelobjs.collision_sprite = 0;
            ocrash.crash_state = 0;
        }
    }

    // Setup Default Ferrari Properties
    spr_ferrari->x = 0;
    spr_ferrari->zoom = 0x7F;
    spr_ferrari->draw_props = oentry::BOTTOM    ; // Anchor Bottom
    spr_ferrari->shadow = 3;
    spr_ferrari->width = 0;
    spr_ferrari->priority = spr_ferrari->road_priority = 0x1FD;

    // Set Ferrari H-Flip Based On Steering & Speed
    int16_t d4 = oinputs.steering_adjust;

    // If steering close to centre clear d4 to ignore h-flip of Ferrari
    if (d4 >= -8 && d4 <= 7)
        d4 = 0;
    // If speed to slow clear d4 to ignore h-flip of Ferrari
    if (oinitengine.car_increment >> 16 < 0x14)
        d4 = 0;

    // cont2:
    d4 >>= 2; // increase change of being close to zero and no h-flip occurring
    
    int16_t x_off = 0;

    // ------------------------------------------------------------------------
    // Not Skidding
    // ------------------------------------------------------------------------
    if (!ocrash.skid_counter)
    {
        if (d4 >= 0)
            spr_ferrari->control &= ~OSprites::HFLIP;
        else
            spr_ferrari->control |= OSprites::HFLIP;

        // 0x9E4E not_skidding:

        // Calculate change in road y, so we can determine incline frame for ferrari
        int16_t y = oroad.road_y[oroad.road_p0 + (0x3D0 / 2)] - oroad.road_y[oroad.road_p0 + (0x3E0 / 2)];

        // Converts y difference to a frame value (this is for when on an incline)
        int16_t incline_frame_offset = 0;
        if (y >= 0x12) incline_frame_offset += 8;
        if (y >= 0x13) incline_frame_offset += 8;

        // Get abs version of ferrari turn
        int16_t turn_frame_offset = 0;
        int16_t d2 = d4;
        if (d2 < 0) d2 = -d2;
        if (d2 >= 0x12) turn_frame_offset += 0x18;
        if (d2 >= 0x1E) turn_frame_offset += 0x18;

        // Set Ferrari Sprite Properties
        uint32_t offset = outrun.adr.sprite_ferrari_frames + turn_frame_offset + incline_frame_offset;
        spr_ferrari->addr = roms.rom0p->read32(offset);     // Set Ferrari Frame Address
        sprite_pass_y = roms.rom0p->read16(offset + 4); // Set Passenger Y Offset
        x_off = roms.rom0p->read16(offset + 6); // Set Ferrari Sprite X Offset

        if (d4 < 0) x_off = -x_off;
    }
    // ------------------------------------------------------------------------
    // Skidding
    // ------------------------------------------------------------------------
    else
    {
        int16_t skid_counter = ocrash.skid_counter;

        if (skid_counter < 0)
        {
            spr_ferrari->control |= OSprites::HFLIP;
            skid_counter = -skid_counter; // Needs to be positive
        }
        else
            spr_ferrari->control &= ~OSprites::HFLIP;

        int16_t frame = 0;

        if (skid_counter >= 3)  frame += 8;
        if (skid_counter >= 6)  frame += 8;
        if (skid_counter >= 12) frame += 8;

        // Calculate incline
        int16_t y = oroad.road_y[oroad.road_p0 + (0x3D0 / 2)] - oroad.road_y[oroad.road_p0 + (0x3E0 / 2)];

        int16_t incline_frame_offset = 0;
        if (y >= 0x12) incline_frame_offset += 0x20;
        if (y >= 0x13) incline_frame_offset += 0x20;

        uint32_t offset = outrun.adr.sprite_skid_frames + frame + incline_frame_offset;
        spr_ferrari->addr = roms.rom0p->read32(offset); // Set Ferrari Frame Address
        sprite_pass_y = roms.rom0p->read16(offset + 4); // Set Passenger Y Offset
        x_off = roms.rom0p->read16(offset + 6);         // Set Ferrari Sprite X Offset
        wheel_traction = TRACTION_OFF;                  // Both wheels have lost traction

        if (ocrash.skid_counter >= 0) x_off = -x_off;
    }

    spr_ferrari->x += x_off;

    shake();
    set_ferrari_palette();
    draw_sprite(spr_ferrari);
}

// Bonus Mode: Setup Ferrari Sprite Details
//
// Source: 0xA212
void OFerrari::setup_ferrari_bonus_sprite()
{
    // Setup Default Ferrari Properties
    spr_ferrari->y = 221;
    //spr_ferrari->x = 0; not really needed as set below
    spr_ferrari->priority = spr_ferrari->road_priority = 0x1FD;

    if (oinputs.steering_adjust > 0)
        spr_ferrari->control &= ~OSprites::HFLIP;
    else
        spr_ferrari->control |= OSprites::HFLIP;

    // Get abs version of ferrari turn
    int16_t turn_frame_offset = 0;
    int16_t d2 = oinputs.steering_adjust >> 2;
    if (d2 < 0) d2 = -d2;
    if (d2 >= 0x4) turn_frame_offset += 0x18;
    if (d2 >= 0x8) turn_frame_offset += 0x18;

    // Set Ferrari Sprite Properties
    uint32_t offset   = outrun.adr.sprite_ferrari_frames + turn_frame_offset + 8; // 8 denotes the 'level' frames, no slope.
    spr_ferrari->addr = roms.rom0p->read32(offset);     // Set Ferrari Frame Address
    sprite_pass_y     = roms.rom0p->read16(offset + 4); // Set Passenger Y Offset
    int16_t x_off     = roms.rom0p->read16(offset + 6); // Set Ferrari Sprite X Offset

    if (oinputs.steering_adjust < 0) x_off = -x_off;
    spr_ferrari->x = x_off;

    set_ferrari_palette();
    //osprites.map_palette(spr_ferrari);
    //osprites.do_spr_order_shadows(spr_ferrari);
    draw_sprite(spr_ferrari);
}

// Source: 0xA1CE
void OFerrari::init_end_seq()
{
    // AI for remainder
    oattractai.check_road_bonus();
    oattractai.set_steering_bonus();
    // Brake Car!
    oinputs.acc_adjust = 0;
    oinputs.brake_adjust = 0xFF;
    do_end_seq();
}

// Drive Ferrari to the side during end sequence
//
// Source: 0xA298
void OFerrari::do_end_seq()
{
    spr_ferrari->y = 221;
    spr_ferrari->priority = spr_ferrari->road_priority = 0x1FD;

    // Set Ferrari Frame
    // +0 [Long]: Address of frame
    // +4 [Byte]: Passenger Offset (always 0!)
    // +5 [Byte]: Ferrari X Change
    // +6 [Byte]: Sprite Colour Palette
    // +7 [Byte]: H-Flip

    uint32_t addr = outrun.adr.anim_ferrari_frames + ((obonus.bonus_control - 0xC) << 1);

    spr_ferrari->addr    = roms.rom0p->read32(addr);
    sprite_pass_y        = roms.rom0p->read8(4 + addr);  // Set Passenger Y Offset
    spr_ferrari->x       = roms.rom0p->read8(5 + addr);
    spr_ferrari->pal_src = roms.rom0p->read8(6 + addr);
    spr_ferrari->control = roms.rom0p->read8(7 + addr) | (spr_ferrari->control & 0xFE); // HFlip

    osprites.map_palette(spr_ferrari);
    osprites.do_spr_order_shadows(spr_ferrari);
}

// - Update Car Palette To Adjust Brake Lamp
// - Also Control Speed at which wheels spin through palette adjustment
//
// Source: 0x9F7C
void OFerrari::set_ferrari_palette()
{
    uint8_t pal;

    // Denote palette for brake light
    if (oinputs.brake_adjust >= OInputs::BRAKE_THRESHOLD1)
    {
        outrun.outputs->set_digital(OOutputs::D_BRAKE_LAMP);
        pal = 2;
    }
    else
    {
        outrun.outputs->clear_digital(OOutputs::D_BRAKE_LAMP);
        pal = 0;
    }

    // Car Moving
    if (oinitengine.car_increment >> 16 != 0)
    {
        int16_t car_increment = 5 - (oinitengine.car_increment >> 21);
        if (car_increment < 0) car_increment = 0;
        wheel_frame_reset = car_increment;

        // Increment wheel palette offset (to move wheels)
        if (wheel_counter <= 0)
        {
            wheel_counter = wheel_frame_reset;
            wheel_pal++; 
        }
        else
            wheel_counter--;
    }
    spr_ferrari->pal_src = pal + 2 + (wheel_pal & 1);
}

// Set Ferrari X Position
//
// - Reads steering value
// - Converts to a practical change in x position
// - There are a number of special cases, as you will see by looking at the code
//
// In use:
//
// d0 = Amount to adjust car x position by
//
// Source: 0xC1B2

void OFerrari::set_ferrari_x()
{
    int16_t steering = oinputs.steering_adjust;

    // Hack to reduce the amount you can steer left and right at the start of Stage 1
    // The amount you can steer increases as you approach road position 0x7F
    if (ostats.cur_stage == 0 && oinitengine.rd_split_state == 0 && (oroad.road_pos >> 16) <= 0x7F)
    {
        steering = (steering * (oroad.road_pos >> 16)) >> 7;
    }

    steering -= steering_old;
    if (steering > 0x40) steering = 0x40;
    else if (steering < -0x40) steering = -0x40;
    steering_old += steering;
    steering = steering_old;

    // This block of code reduces the amount the player
    // can steer left and right, when below a particular speed
    if (wheel_state == OFerrari::WHEELS_ON && (oinitengine.car_increment >> 16) <= 0x7F)
    {
        steering = (steering * (oinitengine.car_increment >> 16)) >> 7;
    }

    // Check Road Curve And Adjust X Value Accordingly
    // This effectively makes it harder to steer into sharp corners
    int16_t road_curve = oinitengine.road_curve;
    if (road_curve)
    {
        road_curve -= 0x40;
        if (road_curve < 0)
        {
            int16_t diff_from_max = (MAX_SPEED >> 17) - (oinitengine.car_increment >> 16);
            if (diff_from_max < 0)
            {
                int16_t curve = diff_from_max * road_curve;
                int32_t result = (int32_t) steering * (0x24C0 - curve);
                steering = result / 0x24C0;
            }
        }
    }

    steering >>= 3;
    int16_t steering2 = steering;
    steering >>= 2;
    steering += steering2;
    steering = -steering;

    // Return if car is not moving
    if (outrun.game_state == GS_INGAME && oinitengine.car_increment >> 16 == 0)
    {
        // Hack so car is centered if we want to bypass countdown sequence
        int16_t road_width_change = (oroad.road_width >> 16) - road_width_old;
        road_width_old = (oroad.road_width >> 16);
        if (oinitengine.car_x_pos < 0)
            road_width_change = -road_width_change;
        oinitengine.car_x_pos += road_width_change;
        // End of Hack
        return;
    }
    
    oinitengine.car_x_pos += steering;
    
    int16_t road_width_change = (oroad.road_width >> 16) - road_width_old;
    road_width_old = (oroad.road_width >> 16);
    if (oinitengine.car_x_pos < 0)
        road_width_change = -road_width_change;
    oinitengine.car_x_pos += road_width_change;
}

// Ensure car does not stray too far from sides of road
// There are three parts to this function:
// a. Normal Road
// b. Road Split
// c. Dual Lanes
//
// Source: 0xC2B0
void OFerrari::set_ferrari_bounds()
{
    // d0 = road_width
    // d2 = car_x_pos

    int16_t road_width16 = oroad.road_width >> 16;
    int16_t d1 = 0;

    // Road Split Both Lanes
    if (oinitengine.rd_split_state == 4)
    {
        if (oinitengine.car_x_pos < 0)
            road_width16 = -road_width16;
        d1 = road_width16 + 0x140;
        road_width16 -= 0x140;
    }
    // One Lane
    else if (road_width16 <= 0xFF)
    {
        road_width16 += OFFROAD_BOUNDS;
        d1 = road_width16;
        road_width16 = -road_width16;
    }
    // Two Lanes - road_two_lanes:
    else
    {
        if (oinitengine.car_x_pos < 0)
            road_width16 = -road_width16;
        d1 = road_width16 + OFFROAD_BOUNDS;
        road_width16 -= OFFROAD_BOUNDS;
    }

    // Set Bounds
    if (oinitengine.car_x_pos < road_width16)
        oinitengine.car_x_pos = road_width16;
    else if (oinitengine.car_x_pos > d1)
        oinitengine.car_x_pos = d1;

    oroad.car_x_bak = oinitengine.car_x_pos;
    oroad.road_width_bak = oroad.road_width >> 16; 
}

// Check Car Is Still On Road
//
// Source: 0xBFDC
void OFerrari::check_wheels()
{
    wheel_state = WHEELS_ON;
    wheel_traction = TRACTION_ON;
    uint16_t road_width = oroad.road_width >> 16;

    switch (oroad.road_ctrl)
    {
        case ORoad::ROAD_OFF:         // Both Roads Off
            return;

        // Single Road
        case ORoad::ROAD_R0:          // Road 0
        case ORoad::ROAD_R1:          // Road 1
        case ORoad::ROAD_R0_SPLIT:    // Road 0 (Road Split.)
        case ORoad::ROAD_R1_SPLIT:    // Road 1 (Road Split. Invert Road 1)
        {
            int16_t x = oinitengine.car_x_pos;
            
            if (oroad.road_ctrl == ORoad::ROAD_R0_SPLIT)
                x -= road_width;
            else    
                x += road_width;

            if (oroad.road_ctrl == ORoad::ROAD_R0_SPLIT || oroad.road_ctrl == ORoad::ROAD_R1_SPLIT)
            {
                if (x > -0xD4 && x <= 0xD4) return;
                else if (x < -0x104 || x > 0x104) set_wheels(WHEELS_OFF);
                else if (x > 0xD4 && x <= 0x104) set_wheels(WHEELS_LEFT_OFF);
                else if (x <= 0xD4 && x >= -0x104) set_wheels(WHEELS_RIGHT_OFF);
            }
            else
            {
                if (x > -0xD4 && x <= 0xD4) return;
                else if (x < -0x104 || x > 0x104) set_wheels(WHEELS_OFF);
                else if (x > 0xD4 && x <= 0x104) set_wheels(WHEELS_RIGHT_OFF);
                else if (x <= 0xD4 && x >= -0x104) set_wheels(WHEELS_LEFT_OFF);
            }
        }
        break;
        
        // Both Roads
        case ORoad::ROAD_BOTH_P0:     // Both Roads (Road 0 Priority) [DEFAULT]
        case ORoad::ROAD_BOTH_P1:     // Both Roads (Road 1 Priority) 
        case ORoad::ROAD_BOTH_P0_INV: // Both Roads (Road 0 Priority) (Road Split. Invert Road 1)
        case ORoad::ROAD_BOTH_P1_INV: // Both Roads (Road 1 Priority) (Road Split. Invert Road 1)
        {
            int16_t x = oinitengine.car_x_pos;

            if (road_width > 0xFF)
            {
                if (x < 0)
                    x += road_width;
                else
                    x -= road_width;

                if (x < -0x104 || x > 0x104) set_wheels(WHEELS_OFF);
                else if (x < -0xD4) set_wheels(WHEELS_RIGHT_OFF);
                else if (x > 0xD4) set_wheels(WHEELS_LEFT_OFF);
            }
            else
            {
                road_width += 0x104;

                if (x >= 0)
                {
                    if (x > road_width) set_wheels(WHEELS_OFF);
                    else if (x > road_width - 0x30) set_wheels(WHEELS_LEFT_OFF);
                }
                else
                {
                    x = -x;
                    if (x > road_width) set_wheels(WHEELS_OFF);
                    else if (x > road_width - 0x30) set_wheels(WHEELS_RIGHT_OFF);
                }
            }
        }
        break;
    }   
}

void OFerrari::set_wheels(uint8_t new_state) 
{
    wheel_state = new_state;
    wheel_traction = (new_state == WHEELS_OFF) ? 2 : 1;
}

// Adjusts the x-position of the car based on the curve.
// This effectively stops the user driving through the courses in a straight line.
// (sticks the car to the track).
//
// Source: 0xBF6E
void OFerrari::set_curve_adjust()
{
    int16_t x_diff = oroad.road_x[170] - oroad.road_x[511];

    // Invert x diff when taking roadsplit
    if (oinitengine.rd_split_state && oinitengine.car_x_pos < 0)
        x_diff = -x_diff;

    x_diff >>= 6;
    
    if (x_diff)
    {
        x_diff *= (oinitengine.car_increment >> 16);
        x_diff /= 0xDC;
        x_diff <<= 1;
        oinitengine.car_x_pos += x_diff;
    }
}

// Draw Shadow below Ferrari
//
// Source: 0xA7BC
void OFerrari::draw_shadow()
{
    if (spr_shadow->control & OSprites::ENABLE)
    {
        if (outrun.game_state == GS_MUSIC) return;

        if (outrun.tick_frame)
        {
            spr_shadow->road_priority = spr_ferrari->road_priority - 1;
            spr_shadow->x = spr_ferrari->x;
            spr_shadow->y = 222;
            spr_shadow->zoom = 0x99;
            spr_shadow->draw_props = 8;
            spr_shadow->addr = outrun.adr.shadow_data;
        }

        if (oroad.get_view_mode() != ORoad::VIEW_INCAR)
            osprites.do_spr_order_shadows(spr_shadow);
    }
}

// Set Passenger Sprite X/Y Position.
//
// Routine is used separately for both male and female passengers.
//
// In use:
// a1 = Passenger XY offset table
// a5 = Passenger 1 / Passenger 2 Sprite
// a6 = Car Sprite
//
// Memory locations:
// 62F00 = Car
// 62F40 = Passenger 1 (Man)
// 62F80 = Passenger 2 (Girl)
//
// Source: A568
void OFerrari::set_passenger_sprite(oentry* sprite)
{
    sprite->road_priority = spr_ferrari->road_priority;
    sprite->priority = spr_ferrari->priority + 1;
    uint16_t frame = sprite_pass_y << 3;

    // Is this a bug in the original? Note that by negating HFLIP check the passengers
    // shift right a few pixels on acceleration.
    if ((oinitengine.car_increment >> 16 >= 0x14) && !(spr_ferrari->control & OSprites::HFLIP))
        frame += 4;

    // --------------------------------------------------------------------------------------------
    // Set Palette
    // --------------------------------------------------------------------------------------------
    uint8_t pal = 0;

    // Test for car collision frame
    if (sprite_pass_y == 9)
    {
        // Set Brown/Blonde Palette depending on whether man or woman
        if (sprite == spr_pass1) pal = 0xA;
        else pal = 0x8;
    }
    else
    {
        if (sprite == spr_pass1) pal = 0x2D;
        else pal = 0x2E;
    }

    // --------------------------------------------------------------------------------------------
    // Set X/Y Position
    // --------------------------------------------------------------------------------------------

    sprite->pal_src = pal;
    uint32_t offset_table = ((sprite == spr_pass1) ? PASS1_OFFSET : PASS2_OFFSET) + frame;
    sprite->x = spr_ferrari->x + roms.rom0.read16(&offset_table);
    sprite->y = spr_ferrari->y + roms.rom0.read16(offset_table);
    
    sprite->zoom = 0x7F;
    sprite->draw_props = 8;
    sprite->shadow = 3;
    sprite->width = 0;

    set_passenger_frame(sprite);
    draw_sprite(sprite);
}

// Set Passenger Sprite Frame
//
// - Set the appropriate male/female frame
// - Uses the car's speed to set the hair frame
// - Also handles skid case
//
// Source: 0xA632

void OFerrari::set_passenger_frame(oentry* sprite)
{
    uint32_t addr = outrun.adr.sprite_pass_frames;
    if (sprite == spr_pass2) addr += 4; // Female frames
    uint16_t inc = oinitengine.car_increment >> 16;

    // Car is moving
    // Use adjusted increment/speed of car as reload value for sprite counter (to ultimately set hair frame)
    if (inc != 0)
    {
        if (inc > 0xFF) inc = 0xFF;
        inc >>= 5;
        int16_t counter = 9 - inc;
        if (counter < 0) counter = 0;
        sprite->reload = counter;
        if (sprite->counter <= 0)
        {
            sprite->counter = sprite->reload;
            sprite->xw1++; // Reuse this as a personal tick counter to flick between hair frames
        }
        else
            sprite->counter--;
        
        inc = (sprite->xw1 & 1) << 3;
    }

    // Check Skid
    if (sprite->pass_props >= 9)
    {
        // skid left
        if (ocrash.skid_counter > 0)
        {
            sprite->addr = (sprite == spr_pass1) ? 
                outrun.adr.sprite_pass1_skidl : outrun.adr.sprite_pass2_skidl;
        }
        // skid right
        else
        {
            sprite->addr = (sprite == spr_pass1) ? 
                outrun.adr.sprite_pass1_skidr : outrun.adr.sprite_pass2_skidr;
        }
    }
    else
        sprite->addr = roms.rom0p->read32(addr + inc);
}

// ------------------------------------------------------------------------------------------------
//                                       CAR HANDLING ROUTINES
// ------------------------------------------------------------------------------------------------

// Main routine handling car speed, revs, torque
//
// Source: 0x6288
void OFerrari::move()
{
    if (car_ctrl_active)
    {      
        // Auto braking if necessary
        if (outrun.game_state != GS_ATTRACT && auto_brake)
            oinputs.acc_adjust = 0;   

        // Set Gear For Demo Mode
        if (FORCE_AI || 
            outrun.game_state == GS_ATTRACT || outrun.game_state == GS_BONUS || 
            config.controls.gear == config.controls.GEAR_AUTO)
        {
            // demo_mode_gear
            oinputs.gear = (oinitengine.car_increment >> 16 > 0xA0);
        }

        gfx_smoke = 0;

        // --------------------------------------------------------------------
        // CRASH CODE - Slow Car
        // --------------------------------------------------------------------
        if (ocrash.crash_counter && ocrash.spin_control1 <= 0)
        {
            oinitengine.car_increment = (oinitengine.car_increment & 0xFFFF) | ((((oinitengine.car_increment >> 16) * 31) >> 5) << 16);
            revs = 0;
            gear_value = 0;
            gear_bak = 0;
            gear_smoke = 0;
            torque = 0x1000;
        }
        // --------------------------------------------------------------------
        // NO CRASH
        // --------------------------------------------------------------------
        else
        {    
            if (car_state >= 0) car_state = CAR_NORMAL; // No crash - Clear smoke from wheels

            // check_time_expired:
            // 631E: Clear acceleration value if time out
            if ((ostats.time_counter & 0xFF) == 0)
                oinputs.acc_adjust = 0;
        
            // --------------------------------------------------------------------
            // Do Car Acceleration / Revs / Torque
            // Note: Torque gets set based on gear car is in
            // --------------------------------------------------------------------
        
            // do_acceleration:
            car_acc_brake();

            int32_t d2 = revs / torque;

            if (!oinitengine.ingame_engine)
            {
                tick_engine_disabled(d2);
            }
            else
            {      
                int16_t d1 = torque_index;

                if (gear_counter == 0)
                    do_gear_torque(d1);
            }
            // set_torque:
            int16_t new_torque = torque_lookup[torque_index];
            torque = new_torque;
            d2 = (int32_t) (d2 & 0xFFFF) * new_torque; // unsigned multiply
            int32_t accel_copy = accel_value << 16;
            int32_t rev_adjust_new = 0;

            if (gear_counter != 0)
                rev_adjust_new = tick_gear_change(d2 >> 16);

            // Compare Accelerator To Proposed New Speed
            //
            // d0 = Desired Accelerator Value [accel_copy]
            // d1 = Torque Value [new_torque]
            // d2 = Proposed New Rev Value [d2]
            // d4 = Rev Adjustment (due to braking / off road etc / revs being higher than desired) [rev_adjust]
            else if (d2 != accel_copy)
            {
                if (accel_copy >= d2)
                    rev_adjust_new = get_speed_inc_value(new_torque, d2);
                else
                    rev_adjust_new = get_speed_dec_value(new_torque);
            }

            // test_smoke:
            if (gear_smoke)
                rev_adjust_new = tick_smoke(); // Note this also changes the speed [stored in d4]

            // cont2:
            set_brake_subtract();               // Calculate brake value to subtract from revs
            finalise_revs(d2, rev_adjust_new);  // Subtract calculated value from revs
            convert_revs_speed(new_torque, d2); // d2 is converted from revs to speed

            // Ingame Control Not Active. Clear Car Speed
            if (!oinitengine.ingame_engine)
            {
                oinitengine.car_increment = 0;
                car_inc_old = 0;
            }
            // Set New Car Speed to car_increment
            else if (outrun.game_state != GS_BONUS)
            {
                int16_t diff = car_inc_old - (d2 >> 16); // Old Speed - New Speed

                // Car is moving
                if (diff != 0)
                {
                    // Car Speeding Up (New Speed is faster)
                    if (diff < 0)
                    {
                        diff = -diff;
                        uint8_t adjust = 2;
                        if (oinitengine.car_increment >> 16 <= 0x28)
                            adjust >>= 1;
                        if (diff > 2)
                            d2 = (car_inc_old + adjust) << 16;                      
                    }
                    // Car Slowing Down
                    else if (diff > 0)
                    {
                        uint8_t adjust = 2;
                        if (brake_subtract)
                            adjust <<= 2;
                        if (diff > adjust)
                            d2 = (car_inc_old - adjust) << 16;
                    }
                }
                oinitengine.car_increment = d2;
            }
            else
                oinitengine.car_increment = d2;
        } // end crash if/else block
        
        // move_car_rev:
        update_road_pos();
        ohud.draw_rev_counter();
    } // end car_ctrl_active

    // Check whether we want to play slip sound
    // check_slip
    if (gfx_smoke)
    {
        car_state = CAR_SMOKE; // Set smoke from car wheels
        if (oinitengine.car_increment >> 16)
        {
            if (slip_sound == sound::STOP_SLIP)
                osoundint.queue_sound(slip_sound = sound::INIT_SLIP);
        }
        else
            osoundint.queue_sound(slip_sound = sound::STOP_SLIP);
    }
    // no_smoke:
    else
    {
        if (slip_sound != sound::STOP_SLIP)
            osoundint.queue_sound(slip_sound = sound::STOP_SLIP);        
    }
    // move_car
    car_inc_old = oinitengine.car_increment >> 16;
    counter++;
    
    // During Countdown: Clear Car Speed
    if (outrun.game_state == GS_START1 || outrun.game_state == GS_START2 || outrun.game_state == GS_START3)
    {
        oinitengine.car_increment = 0;
        car_inc_old = 0;
    }
    
}

// Handle revs/torque when in-game engine disabled
//
// Source: 0x6694
void OFerrari::tick_engine_disabled(int32_t &d2)
{
    torque_index = 0;
    
    // Crash taking place - do counter and set game engine when expired
    if (ocrash.coll_count1)
    {
        olevelobjs.spray_counter = 0;
        if (--oinitengine.ingame_counter != 0)
            return;
    }
    else if (outrun.game_state != GS_ATTRACT && outrun.game_state != GS_INGAME) 
        return;

    // Switch back to in-game engine mode
    oinitengine.ingame_engine = true;

    torque = 0x1000;

    // Use top word of revs for lookup
    int16_t lookup = (revs >> 16);
    if (lookup > 0xFF) lookup = 0xFF;

    torque_index = (0x30 - rev_inc_lookup[lookup]) >> 2;

    int16_t acc = accel_value - 0x10;
    if (acc < 0) acc = 0;
    acc_post_stop = acc;
    revs_post_stop = lookup;

    // Clear bottom word of d2 and swap
    d2 = d2 << 16;

    //lookup -= 0x10;
    //if (lookup < 0) lookup = 0;
    rev_stop_flag = 14;
}

// - Convert the already processed accleration inputs into a meaningful value for both ACC and BRAKE
// - Adjust acceleration based on number of wheels on road
// - Adjust acceleration if car skidding or spinning
//
// Source: 0x6524
void OFerrari::car_acc_brake()
{
    // ------------------------------------------------------------------------
    // Acceleration
    // ------------------------------------------------------------------------
    int16_t acc1 = oinputs.acc_adjust;
    int16_t acc2 = oinputs.acc_adjust;

    acc1 += acc_adjust1 + acc_adjust2 + acc_adjust3;
    acc1 >>= 2;
    acc_adjust3 = acc_adjust2;
    acc_adjust2 = acc_adjust1;
    acc_adjust1 = acc2;

    if (!oinitengine.ingame_engine)
    {
        acc2 -= accel_value_bak;
        if (acc2 < 0) acc2 = -acc2;
        if (acc2 < 8)
            acc1 = accel_value_bak;
    }
    // test_skid_spin:

    // Clear acceleration on skid
    if (ocrash.spin_control1 > 0 || ocrash.skid_counter)
        acc1 = 0;

    // Adjust speed when offroad
    else if (wheel_state != WHEELS_ON)
    {
        if (gear_value)
            acc1 = (acc1 * 3) / 10;
        else
            acc1 = (acc1 * 6) / 10;

        // If only one wheel off road, increase acceleration by a bit more than if both wheels off-road
        if (wheel_state != WHEELS_OFF)
            acc1 = (acc1 << 1) + (acc1 >> 1);
    }

    // finalise_acc_value:
    accel_value = acc1;
    accel_value_bak = acc1;

    // ------------------------------------------------------------------------
    // Brake
    // ------------------------------------------------------------------------
    int16_t brake1 = oinputs.brake_adjust;
    int16_t brake2 = oinputs.brake_adjust;
    brake1 += brake_adjust1 + brake_adjust2 + brake_adjust3;
    brake1 >>= 2;
    brake_adjust3 = brake_adjust2;
    brake_adjust2 = brake_adjust1;
    brake_adjust1 = brake2;
    brake_value = brake1;

    // ------------------------------------------------------------------------
    // Gears
    // ------------------------------------------------------------------------
    gear_bak = gear_value;
    gear_value = oinputs.gear;
}

// Do Gear Changes & Torque Index Settings
//
// Source: 0x6948
void OFerrari::do_gear_torque(int16_t &d1)
{
    if (oinitengine.ingame_engine)
    {
        d1 = torque_index;
        if (gear_value)
            do_gear_high(d1);
        else
            do_gear_low(d1);
    }
    torque_index = (uint8_t) d1;
    // Backup gear value for next iteration (so we can tell when gear has recently changed)
    gear_bak = gear_value;
}

void OFerrari::do_gear_low(int16_t &d1)
{
    // Recent Shift from high to low
    if (gear_bak)
    {
        gear_value = false;
        gear_counter = 4;
        return;
    }

    // Low Gear - Show Smoke When Accelerating From Standstill
    if (oinitengine.car_increment >> 16 < 0x50 && accel_value - 0xE0 >= 0)
        gfx_smoke++;

    // Adjust Torque Index
    if (d1 == 0x10) return;
    else if (d1 < 0x10)
    {
        d1++;
        return;
    }
    d1 -= 4;
    if (d1 < 0x10)
        d1 = 0x10;
}

void OFerrari::do_gear_high(int16_t &d1)
{
    // Change from Low Gear to High Gear
    if (!gear_bak)
    {
        gear_value = true;
        gear_counter = 4;
        return;
    }

    // Increment torque until it reaches 0x1F
    if (d1 == 0x1F) return;
    d1++;
}

// Source: 0x6752
int32_t OFerrari::tick_gear_change(int16_t rem)
{
    gear_counter--;
    rev_adjust = rev_adjust - (rev_adjust >> 4);

    // Setup smoke when gear counter hits zero
    if (gear_counter == 0)
    {
        rem -= 0xE0;
        if (rem < 0)  return rev_adjust;
        int16_t acc = accel_value - 0xE0;
        if (acc < 0)  return rev_adjust;
        gear_smoke = acc;
    }

    return rev_adjust;
}

// Set value to increment speed by, when revs lower than acceleration
//
// Inputs:
//
// d1 = Torque Value [new_torque]
// d2 = Proposed new increment value [new_rev]
//
// Outputs: d4 [Rev Adjustment]
//
// Source: 679C

int32_t OFerrari::get_speed_inc_value(uint16_t new_torque, uint32_t new_rev)
{
    // Use Top Word Of Revs For Table Lookup. Cap At 0xFF Max
    uint16_t lookup = (new_rev >> 16);
    if (lookup > 0xFF) lookup = 0xFF;

    uint32_t rev_adjust = rev_inc_lookup[lookup]; // d4

    // Double adjustment if car moving slowly
    if (oinitengine.car_increment >> 16 <= 0x14)
        rev_adjust <<= 1;

    rev_adjust = ((new_torque * new_torque) >> 12) * rev_adjust;
    
    if (!oinitengine.ingame_engine) return rev_adjust;
    return rev_adjust << rev_shift;
}

// Set value to decrement speed by, when revs higher than acceleration
//
// Inputs:
//
// d1 = Torque Value [new_torque]
//
// Outputs: d4 [Rev Adjustment]
//
// Source: 67E4
int32_t OFerrari::get_speed_dec_value(uint16_t new_torque)
{
    int32_t new_rev = -((0x440 * new_torque) >> 4);
    if (wheel_state == WHEELS_ON) return new_rev;
    return new_rev << 2;
}

// - Reads Brake Value
// - Translates Into A Value To Subtract From Car Speed
// - Also handles setting smoke on wheels during brake/skid
// Source: 0x6A10
void OFerrari::set_brake_subtract()
{
    int32_t d6 = 0;
    const int32_t DEC = -0x8800; // Base value to subtract from acceleration burst
    
    // Not skidding or spinning
    if (ocrash.skid_counter == 0 && ocrash.spin_control1 == 0)
    {
        if (brake_value < OInputs::BRAKE_THRESHOLD1)
        {
            brake_subtract = d6;
            return;
        }
        else if (brake_value < OInputs::BRAKE_THRESHOLD2)
        {
            brake_subtract = d6 + DEC;
            return;
        }
        else if (brake_value < OInputs::BRAKE_THRESHOLD3)
        {
            brake_subtract = d6 + (DEC * 3);
            return;
        }
        else if (brake_value < OInputs::BRAKE_THRESHOLD4)
        {
            brake_subtract = d6 + (DEC * 5);
            return;
        }
    }
    // check_smoke
    if (oinitengine.car_increment >> 16 > 0x28)
        gfx_smoke++;

    brake_subtract = d6 + (DEC * 9);
}


// - Finalise rev value, taking adjustments into account
//
// Inputs:
//
// d2 = Current Revs
// d4 = Rev Adjustment
//
// Outputs:
//
// d2 = New rev value
//
// Source: 0x67FC

void OFerrari::finalise_revs(int32_t &d2, int32_t rev_adjust_new)
{
    rev_adjust_new += brake_subtract;
    if (rev_adjust_new < -0x44000) rev_adjust_new = -0x44000;
    d2 += rev_adjust_new;
    rev_adjust = rev_adjust_new;
    
    if (d2 > 0x13C0000) d2 = 0x13C0000;
    else if (d2 < 0) d2 = 0;
}

// Convert revs to final speed value
// Set correct pitch for sound fx
//
// Note - main problem seems to be that the revs fed to this routine are wrong in the first place on restart
// rev_stop_flag, revs_post_stop, accel_value and acc_post_stop seem ok?
//
// Source: 0x6838
void OFerrari::convert_revs_speed(int32_t new_torque, int32_t &d2)
{
    revs = d2;
    int32_t d3 = d2;
    if (d3 < 0x1F0000) d3 = 0x1F0000;

    int16_t revs_top = d3 >> 16;
    
    // Check whether we're switching back to ingame engine (after disabling user control of car)
    if (rev_stop_flag)
    {
        if (revs_top >= revs_post_stop)
        {
            rev_stop_flag = 0;
        }
        else
        {
            if (accel_value < acc_post_stop)
                revs_post_stop -= rev_stop_flag;

            // cont1:

            int16_t d5 = revs_post_stop >> 1;
            int16_t d4 = rev_stop_flag;
            if (revs_top >= d5)
            {
                d5 >>= 1;
                d4 >>= 1;
                if (revs_top >= d5)
                    d4 >>= 1;
            }
            // 689c
            revs_post_stop -= d4;
            if (revs_post_stop < 0x1F) revs_post_stop = 0x1F;
            revs_top = revs_post_stop;
        }
    }

    // setup_pitch_fx:
    rev_pitch1 = (revs_top * 0x1A90) >> 8;

    // Setup New Car Increment Speed
    d2 = ((d2 >> 16) * 0x1A90) >> 8;
    d2 = (d2 << 16) >> 4;
    
    /*if (!new_torque)
    {
        std::cout << "convert_revs_speed error!" << std::endl;
    }*/

    d2 = (d2 / new_torque) * 0x480;
    
    if (d2 < 0) d2 = 0;
    else if (d2 > MAX_SPEED) d2 = MAX_SPEED;
}

// Update road_pos, taking road_curve into account
//
// Source: 0x6ABA
void OFerrari::update_road_pos()
{
    uint32_t car_inc = CAR_BASE_INC;

    // Bendy Roads
    if (oinitengine.road_type > OInitEngine::ROAD_STRAIGHT)
    {
        int32_t x = oinitengine.car_x_pos;
        
        if (oinitengine.road_type == OInitEngine::ROAD_RIGHT)
            x = -x;

        x = (x / 0x28) + oinitengine.road_curve;

        car_inc = (car_inc * x) / oinitengine.road_curve;
    }

    car_inc *= oinitengine.car_increment >> 16;
    oroad.road_pos_change = car_inc;
    oroad.road_pos += car_inc;
}

// Decrement Smoke Counter
// Source: 0x666A
int32_t OFerrari::tick_smoke()
{
    gear_smoke--;
    int32_t r = rev_adjust - (rev_adjust >> 4);
    if (gear_smoke >= 8) gfx_smoke++; // Trigger smoke
    return r;
}

// Calculate car score and sound. Strange combination, but there you go!
//
// Source: 0xBD78
void OFerrari::do_sound_score_slip()
{
    // ------------------------------------------------------------------------
    // ENGINE PITCH SOUND
    // ------------------------------------------------------------------------
    uint16_t engine_pitch = 0;

    // Do Engine Rev
    if (outrun.game_state >= GS_START1 && outrun.game_state <= GS_INGAME)
    {
        engine_pitch = rev_pitch2 + (rev_pitch2 >> 1);
    }

    osoundint.engine_data[sound::ENGINE_PITCH_H] = engine_pitch >> 8;
    osoundint.engine_data[sound::ENGINE_PITCH_L] = engine_pitch & 0xFF;

    // Curved Road
    if (oinitengine.road_type != OInitEngine::ROAD_STRAIGHT)
    {
        int16_t steering = oinputs.steering_adjust;
        if (steering < 0) steering = -steering;

        // Hard turn
        if (steering >= 0x70)
        {
            // Move Left
            if (oinitengine.car_x_pos > oinitengine.car_x_old)
                cornering = (oinitengine.road_type == OInitEngine::ROAD_LEFT) ? 0 : -1;
            // Straight
            else if (oinitengine.car_x_pos == oinitengine.car_x_old)
                cornering = 0;
            // Move Right
            else
                cornering = (oinitengine.road_type == OInitEngine::ROAD_RIGHT) ? 0 : -1;
        }
        else
            cornering = 0;
    }
    else
        cornering = 0;

    // update_score:
    car_x_diff = oinitengine.car_x_pos - oinitengine.car_x_old;
    oinitengine.car_x_old = oinitengine.car_x_pos;

    if (outrun.game_state == GS_ATTRACT) 
        return;

    // Wheels onroad - Convert Speed To Score
    if (wheel_state == WHEELS_ON) 
    {
        ostats.convert_speed_score(oinitengine.car_increment >> 16);
    }

    // 0xBE6E
    if (!sprite_slip_copy)
    {
        if (ocrash.skid_counter)
        {
            is_slipping = -1;
            osoundint.queue_sound(sound::INIT_SLIP);
        }
    }
    // 0xBE94
    else
    {
        if (!ocrash.skid_counter)
        {
            is_slipping = 0;
            osoundint.queue_sound(sound::STOP_SLIP);
        }
    }
    // 0xBEAC
    sprite_slip_copy = ocrash.skid_counter;

    // ------------------------------------------------------------------------
    // Switch to cornering. Play Slip Sound. Init Smoke
    // ------------------------------------------------------------------------
    if (!ocrash.skid_counter)
    {
        // 0xBEBE: Initalize Slip
        if (!cornering_old)
        {
            if (cornering)
            {
                is_slipping = -1;
                osoundint.queue_sound(sound::INIT_SLIP);
            }
        }
        // 0xBEE4: Stop Cornering   
        else
        {
            if (!cornering)
            {
                is_slipping = 0;
                osoundint.queue_sound(sound::STOP_SLIP);
            }
        }
    }
    // check_wheels:
    cornering_old = cornering;

    if (sprite_wheel_state)
    {
        // If previous wheels on-road & current wheels off-road - play safety zone sound
        if (!wheel_state)
            osoundint.queue_sound(sound::STOP_SAFETYZONE);
    }
    // Stop Safety Sound
    else
    {
        if (wheel_state)
            osoundint.queue_sound(sound::INIT_SAFETYZONE);
    }
    sprite_wheel_state = wheel_state;
}

// Shake Ferrari by altering XY Position when wheels are off-road
//
// Source: 0x9FEE
void OFerrari::shake()
{
    if (outrun.game_state != GS_INGAME && outrun.game_state != GS_ATTRACT) return;

    if (wheel_traction == TRACTION_ON) return; // Return if both wheels have traction

    int8_t traction = wheel_traction - 1;
    int16_t rnd = outils::random();
    spr_ferrari->counter++;

    uint16_t car_inc = oinitengine.car_increment >> 16; // [d5]
    
    if (car_inc <= 0xA) return; // Do not shake car at low speeds

    if (car_inc <= (0x3C >> traction))
    {
        rnd &= 3;
        if (rnd != 0) return;
    }
    else if (car_inc <= (0x78 >> traction))
    {
        rnd &= 1;
        if (rnd != 0) return;
    }

    if (rnd < 0) spr_ferrari->y--;
    else spr_ferrari->y++;

    rnd &= 1;
    
    if (!(spr_ferrari->counter & BIT_1))
        rnd = -rnd;

    spr_ferrari->x += rnd; // Increment Ferrari X by 1 or -1
}

// Perform Skid (After Car Has Collided With Another Vehicle)
//
// Source: 0xBFB4
void OFerrari::do_skid()
{
    if (!ocrash.skid_counter) return;

    if (ocrash.skid_counter > 0)
    {
        ocrash.skid_counter--;
        oinitengine.car_x_pos += OCrash::SKID_X_ADJ;
    }
    else
    {
        ocrash.skid_counter++;
        oinitengine.car_x_pos -= OCrash::SKID_X_ADJ;
    }
}

void OFerrari::draw_sprite(oentry* sprite)
{
    if (oroad.get_view_mode() != ORoad::VIEW_INCAR)
    {
        osprites.map_palette(sprite);
        osprites.do_spr_order_shadows(sprite);
    }
}

// Rev Lookup Table. 255 Values.
// Used to provide rev adjustment. Note that values tail off at higher speeds.
const uint8_t OFerrari::rev_inc_lookup[] = 
{
    0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17, 
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 
    0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 
    0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 
    0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 
    0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 
    0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 
    0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x20, 0x20, 0x20, 0x20, 
    0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27, 0x28, 0x28, 
    0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B, 0x2B, 0x2C, 0x2C, 0x2C, 0x2D, 0x2D, 0x2D, 0x2E, 0x2E, 0x2E, 
    0x2F, 0x2F, 0x2F, 0x2F, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x2F, 0x2F, 0x2F, 0x2F, 
    0x2E, 0x2E, 0x2E, 0x2D, 0x2D, 0x2D, 0x2C, 0x2C, 0x2B, 0x2B, 0x2A, 0x2A, 0x29, 0x29, 0x28, 0x28, 
    0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20, 0x1F, 0x1E, 0x1D, 0x1C, 0x1B, 0x1A, 0x19, 0x18, 
    0x17, 0x15, 0x13, 0x11, 0x0F, 0x0D, 0x0B, 0x0A, 0x09, 0x09, 0x08, 0x08, 0x07, 0x07, 0x06, 0x06, 
    0x05, 0x05, 0x05, 0x04, 0x04, 0x04, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01
};


const uint16_t OFerrari::torque_lookup[] = 
{
    0x2600, // Offset 0x00 - Start line                                                           
    0x243C, // ..
    0x2278, // values only used when pulling away from start line
    0x20B4,
    0x1EF0,
    0x1D2C,
    0x1B68,
    0x19A4,
    0x17E0,
    0x161C,
    0x1458,
    0x1294,
    0x10D0,
    0xF0C,
    0xD48,
    0xB84,
    0x9BB, // Offset 0x10 - Low Gear
    0x983, // ..
    0x94B, // .. in between these values
    0x913, // .. is the lookup as we shift between
    0x8DB, // .. the two gears
    0x8A3, // ..
    0x86B,
    0x833,
    0x7FB,
    0x7C2,
    0x789,
    0x750,
    0x717,
    0x6DE,
    0x6A5,
    0x66C, // Offset 0x1F - High Gear
};