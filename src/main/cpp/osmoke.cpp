#include "osmoke.hpp"

OSmoke osmoke;

OSmoke::OSmoke(void)
{
}

OSmoke::~OSmoke(void)
{
}

void OSmoke::init()
{
    load_smoke_data = 0;
}

// Called twice, once for each plume of smoke from the car
//
// -------------------------------------
// Animation Data Format For Smoke/Spray
// -------------------------------------
//
// Format:
//
// [+0] Long: Sprite Data Address
// [+4] Byte: Sprite Z value of smoke (bigger value means in front of car, and zoomed further)
// [+5] Byte: Sprite Palette
// [+6] Byte: Sprite X (Bottom 4 bits) 
//            Sprite Y (Top 4 bits)
// [+7] Byte: Bit 0: H-Flip sprite
//            Bit 1: Zoom shift value
//            Bits 4-7: Priority Change Per Frame


// Source: 0xA816
void OSmoke::draw_ferrari_smoke(oentry *sprite)
{
    setup_smoke_sprite(false);

    if (outrun.game_state != GS_ATTRACT)
    {
        if (outrun.game_state < GS_START1 || outrun.game_state >= GS_INIT_GAMEOVER) return; 
    }

    if (ocrash.crash_counter && !ocrash.crash_z) return;

    // ------------------------------------------------------------------------
    // Spray from water. More violent than the offroad wheel stuff
    // ------------------------------------------------------------------------

    if (olevelobjs.spray_counter)
    {
        tick_smoke_anim(sprite, 1, roms.rom0.read32(SPRAY_DATA + olevelobjs.spray_type));
        return;
    }
    
    // ------------------------------------------------------------------------
    // Car Slipping/Skidding
    // ------------------------------------------------------------------------

    if (oferrari.is_slipping && oferrari.wheel_state == OFerrari::WHEELS_ON)
    {
        tick_smoke_anim(sprite, 0, roms.rom0.read32(SMOKE_DATA + smoke_type_slip));
        return;
    }

    // ------------------------------------------------------------------------
    // Wheels Offroad
    // ------------------------------------------------------------------------

    if (oferrari.wheel_state != OFerrari::WHEELS_ON)
    {
        // Left Wheel Only
        if (sprite == &osprites.jump_table[OSprites::SPRITE_SMOKE2] && oferrari.wheel_state == OFerrari::WHEELS_LEFT_OFF)
            tick_smoke_anim(sprite, 1, roms.rom0.read32(SMOKE_DATA + smoke_type_offroad));

        // Right Wheel Only
        else if (sprite == &osprites.jump_table[OSprites::SPRITE_SMOKE1] && oferrari.wheel_state == OFerrari::WHEELS_RIGHT_OFF)
            tick_smoke_anim(sprite, 1, roms.rom0.read32(SMOKE_DATA + smoke_type_offroad));
        
        // Both Wheels
        else if (oferrari.wheel_state == OFerrari::WHEELS_OFF)
            tick_smoke_anim(sprite, 1, roms.rom0.read32(SMOKE_DATA + smoke_type_offroad));

        return;
    }

    // test_crash_intro:
    
    // Normal
    if (oferrari.car_state == OFerrari::CAR_NORMAL)
    {
        sprite->type = sprite->xw1; // Copy frame number to type
    }
    // Smoke from wheels
    else if (oferrari.car_state == OFerrari::CAR_SMOKE)
    {
        tick_smoke_anim(sprite, 1, roms.rom0.read32(SMOKE_DATA + smoke_type_onroad));
    }
    // Animation Sequence
    else
    {
        if (oferrari.wheel_state != OFerrari::WHEELS_ON)
            sprite->type = sprite->xw1; // Copy frame number to type
        else
        {
            tick_smoke_anim(sprite, 1, roms.rom0.read32(SMOKE_DATA + smoke_type_onroad));
        }
    }
}

// - Set wheel spray sprite data dependent on upcoming stage
// - Use Main entry point when we know for a fact road isn't splitting
// - Use SetSmokeSprite1 entry point when road could potentially be splitting
//   Source: 0xA94C
void OSmoke::setup_smoke_sprite(bool force_load)
{
    uint16_t stage_lookup = 0;

    // Check whether we should load new sprite data when transitioning between stages
    if (!force_load)
    {
        bool set = load_smoke_data & BIT_0;
        load_smoke_data &= ~BIT_0;
        if (!set) return; // Don't load new smoke data
        stage_lookup = oroad.stage_lookup_off + 8;
    }

    // Set Smoke Colour When On Road
    const uint8_t ONROAD_SMOKE[] = 
    { 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Stage 1
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Stage 2
        0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, // Stage 3
        0x00, 0x00, 0x0A, 0x07, 0x00, 0x00, 0x00, 0x00, // Stage 4
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Stage 5
    };

    smoke_type_onroad = ONROAD_SMOKE[stage_lookup] << 2;
    smoke_type_slip = smoke_type_onroad;

    // Set Smoke Colour When Off Road
    const uint8_t OFFROAD_SMOKE[] = 
    { 
        0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Stage 1
        0x09, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Stage 2
        0x02, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, // Stage 3
        0x08, 0x05, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00, // Stage 4
        0x08, 0x02, 0x08, 0x06, 0x09, 0x00, 0x00, 0x00  // Stage 5
    };

    smoke_type_offroad = OFFROAD_SMOKE[stage_lookup] << 2;
}

// A relatively lengthy routine to set the smoke x,y and z values
// Also sets the speed at which the animation repeats
//
// Inputs:
//
// d0 = 0: Use car speed to determine animation speed
//    = 1: Use revs to determine animation speed
//
// a5 = Smoke Sprite Plume
// Source: 0xA9B6
void OSmoke::tick_smoke_anim(oentry* sprite, int8_t anim_ctrl, uint32_t addr)
{
    sprite->x = oferrari.spr_ferrari->x;
    sprite->y = oferrari.spr_ferrari->y;

    // ------------------------------------------------------------------------
    // Use revs to set sprite counter reload value
    // ------------------------------------------------------------------------
    if (anim_ctrl == 1)
    {
        int16_t revs = 0;

        // Force smoke during animation sequence
        if (oferrari.car_state == OFerrari::CAR_ANIM_SEQ)
            revs = 0x80;
        else
        {
            revs = oferrari.revs >> 16;
            if (revs > 0xFF) revs = 0xFF;
        }

        sprite->reload = 3 - (revs >> 6);
        sprite->z = (revs >> 1);           // More revs = smoke is emitted further

        // Crash Occuring
        if (ocrash.crash_counter)
        {
            sprite->y = -(oroad.road_y[oroad.road_p0 + ocrash.crash_z] >> 4) + 223;

            // Trigger Smoke Cloud.
            // Car is slid to the side, so we need to offset the smoke accordingly
            if (ocrash.crash_state == 4)
            {
                if (ocrash.spr_ferrari->control & OSprites::HFLIP)
                {
                    if (sprite == &osprites.jump_table[OSprites::SPRITE_SMOKE2])
                    {
                        sprite->y -= 10;
                    }
                    else
                    {
                        sprite->x -= 64;
                        sprite->y -= 4;
                    }
                }
                else
                {
                    if (sprite == &osprites.jump_table[OSprites::SPRITE_SMOKE2])
                    {
                        sprite->x += 64;
                        sprite->y -= 4;
                    }
                    else
                    {
                        sprite->y -= 10;    
                    }
                }
            } // End trigger smoke cloud

            int16_t z_shift = ocrash.crash_spin_count - 1;
            if (z_shift == 0) z_shift = 1;
            sprite->z = 0xFF >> z_shift;
        } 
    }
    // ------------------------------------------------------------------------
    // Use car speed to set sprite counter reload value
    // ------------------------------------------------------------------------
    else
    {
        uint16_t car_inc = (oinitengine.car_increment >> 16);
        if (car_inc > 0xFF) car_inc = 0xFF;
        sprite->reload = 7 - (car_inc >> 5);
        sprite->z = 0;
    }

    // Return if stationary and not in animation sequence
    if (oferrari.car_state != OFerrari::CAR_ANIM_SEQ && oinitengine.car_increment >> 16 == 0)
        return; 

    if (outrun.tick_frame)
    {
        if (sprite->counter > 0)
        {
            sprite->counter--;
        }
        else
        {
            // One Wheel Off Road
            if (oferrari.wheel_state != OFerrari::WHEELS_ON && oferrari.wheel_state != OFerrari::WHEELS_OFF)
            {
                sprite->counter = sprite->reload;
                sprite->xw1++; // Increment Frame
            }
            // Two Wheels On Road
            else if (sprite == &osprites.jump_table[OSprites::SPRITE_SMOKE1])
            {
                sprite->counter = sprite->reload;
                sprite->xw1++; // Increment Frame

                // Copy to second smoke sprite (yes this is crap, but it's directly ported code)
                osprites.jump_table[OSprites::SPRITE_SMOKE2].counter = sprite->reload;
                osprites.jump_table[OSprites::SPRITE_SMOKE2].xw1 = sprite->xw1;
            }
        }
    }
    // setup_smoke:
    uint16_t frame   = (sprite->xw1 & 7) << 3;
    sprite->addr     = roms.rom0.read32(addr + frame);
    sprite->pal_src  = roms.rom0.read8(addr + frame + 5);
    uint16_t smoke_z = roms.rom0.read8(addr + frame + 4) + sprite->z;
    if (smoke_z > 0xFF) smoke_z = 0xFF;

    // inc_crash_z:
    // When setting up smoke, include crash_z value into zoom if necessary
    if (ocrash.crash_counter && ocrash.crash_z)
    {
        smoke_z = (smoke_z * ocrash.crash_z) >> 9;
    }

    // Set Sprite Zoom
    if (smoke_z <= 0x40) smoke_z = 0x40;
    uint8_t shift = (roms.rom0.read8(addr + frame + 7) & 2) >> 1;
    uint8_t zoom = smoke_z >> shift;
    if (zoom <= 0x40) zoom = 0x40;
    sprite->zoom = zoom;

    // Set Sprite Y
    sprite->y += ((roms.rom0.read8(addr + frame + 6) & 0xF) * zoom) >> 8;

    // Set Sprite Priority
    sprite->priority = oferrari.spr_ferrari->priority + ((roms.rom0.read8(addr + frame + 7) >> 4) & 0xF);
    sprite->road_priority = sprite->priority;

    // Set Sprite X
    uint8_t hflip = (roms.rom0.read8(addr + frame + 7) & 1);
    int8_t x = ((roms.rom0.read8(addr + frame + 6) >> 3) & 0x1E);

    if (sprite == &osprites.jump_table[OSprites::SPRITE_SMOKE1])
    {
        sprite->draw_props = oentry::BOTTOM | oentry::LEFT; // Anchor bottom left
        hflip++;
    }
    else
    {
        sprite->draw_props = oentry::BOTTOM | oentry::RIGHT; // Anchor bottom right
        x = -x;        
    }
    sprite->x += (x * zoom) >> 8;

    // Set H-Flip
    if (hflip & 1) sprite->control |= OSprites::HFLIP;
    else sprite->control &= ~OSprites::HFLIP;

    osprites.map_palette(sprite);
    osprites.do_spr_order_shadows(sprite);
}

// Draw only helper routine.
// Useful for framerate changes.
void OSmoke::draw(oentry* sprite)
{
    if (outrun.game_state != GS_ATTRACT)
    {
        if (outrun.game_state < GS_START1 || outrun.game_state >= GS_INIT_GAMEOVER) return; 
    }

    if (ocrash.crash_counter && !ocrash.crash_z) return;

    // Return if stationary and not in animation sequence
    if (oferrari.car_state != OFerrari::CAR_ANIM_SEQ && oinitengine.car_increment >> 16 == 0)
        return; 
    
    osprites.map_palette(sprite);
    osprites.do_spr_order_shadows(sprite);
}