#include "engine/otraffic.hpp"

OTraffic otraffic;

OTraffic::OTraffic(void)
{
}


OTraffic::~OTraffic(void)
{
}

void OTraffic::init()
{
    ai_traffic = 0;
    bonus_lhs = 0;
    traffic_split = 0;
    collision_traffic = 0;
    collision_mask = 0;

    traffic_speed_total = 0;
    traffic_speed_avg = 0;
    traffic_pal_cycle = 0;
    traffic_count = 0;
    spawn_counter = 0;
    spawn_location = 0;
    // Set wheel animation reset value across all traffic (moved from spawn traffic routine)
    wheel_counter = wheel_reset = 12;
}

// Disable Traffic Routines
// Source: 0x4A78
void OTraffic::disable_traffic()
{
    for (uint8_t i = OSprites::SPRITE_TRAFF1; i <= OSprites::SPRITE_TRAFF8; i++)
        osprites.jump_table[i].control &= ~OSprites::ENABLE;
}

// Master Function to determine when to spawn traffic
//
// 1. Toggle animation frame to control wheels of traffic
// 2. Spawn traffic when appropriate
//
// Source: 0x4AC8
void OTraffic::spawn_traffic()
{
    if (obonus.bonus_control || 
        outrun.game_state == GS_MAP || outrun.game_state == GS_MUSIC || outrun.game_state == GS_BEST2) 
        return;
    
    spawn_counter++;
    ai_traffic = 0; // Clear AI Traffic Marker
    
    // Use average speed of traffic as new counter reset value to control speed of wheel animations
    if (traffic_speed_avg)
    {
        wheel_reset = -((traffic_speed_avg >> 5) - 11);
        
        if (--wheel_counter == 0)
        {
            wheel_counter = wheel_reset;
            traffic_pal_cycle = 0;
        }
        else if ((wheel_reset >> 1) == wheel_counter)
        {
            traffic_pal_cycle = 1;
        }
    }
    // check_traffic_count
    if (traffic_count >= max_traffic)
        return;

    // Use counter as a spawning delay
    if (! (((spawn_counter - 1) ^ spawn_counter) & BIT_5) )
        return;

    // Spawn Traffic if possible in one of the eight slots
    for (uint8_t i = OSprites::SPRITE_TRAFF1; i <= OSprites::SPRITE_TRAFF8; i++)
    {
        oentry* sprite = &osprites.jump_table[i];
        
        if (!(sprite->control & OSprites::ENABLE))
        {
            spawn_car(sprite);
            return;
        }
    }
}

// Spawn individual vehicle. Called by master function.
// Cars are spawned on the horizon
//
// Source: 0x4BAC
void OTraffic::spawn_car(oentry* sprite)
{
    sprite->control |= OSprites::ENABLE | OSprites::TRAFFIC_SPRITE;
    sprite->draw_props = oentry::BOTTOM;
    sprite->shadow = 7;     // Used as priority
    sprite->width = 0;
    sprite->traffic_proximity = 0;
    sprite->traffic_fx = 0;
    sprite->z = 0x10000;    // Traffic starts on horizon in the distance
    int16_t rnd = outils::random();
    spawn_location++;
    
    // Spawn On Left Hand Side Of Road
    if (spawn_location & 1)
    {
        const int8_t TABLE[] = {0, -0x70, -0x70, 0x70};
        sprite->control &= ~OSprites::TRAFFIC_RHS;
        // note we use (rnd & 6) >> 1 rather than (rnd & 3) to match original random number generation
        sprite->xw1 = sprite->xw2 = TABLE[(rnd & 6) >> 1];  
        sprite->control |= OSprites::HFLIP;   
    }
    // Spawn On Right Hand Side Of Road
    else
    {
        const int8_t TABLE[] = {0, -0x70, 0x70, 0x70};
        sprite->control |= OSprites::TRAFFIC_RHS;
        sprite->xw1 = sprite->xw2 = TABLE[(rnd & 6) >> 1];
        sprite->control &= ~OSprites::HFLIP;
    }
    
    rnd = (int8_t) rnd; // ext.w

    sprite->traffic_orig_speed = (rnd >> 2) + 200;

    // hack////////////////////////////////////////////////////////////////////////////
    //sprite->traffic_orig_speed = 1;
    //traffic_speed_avg = 0;
    // hack////////////////////////////////////////////////////////////////////////////

    sprite->traffic_speed = traffic_speed_avg;

    // Randomize Type of traffic to spawn
    uint8_t spawn_index = (rnd >> 2) + 0x20;

    const int8_t TYPE[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x00, 0x01, 0x02, 0x03, 0x06, 0x07, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0A, 0x0B, 0x08, 0x09, 0x0A, 0x0B, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0C, 0x0D, 0x0C, 0x0D,
        0x0C, 0x0D, 0x0C, 0x0D, 0x0E, 0x0F, 0x0E, 0x0F, 0x0E, 0x0F, 0x0E, 0x0F, 0x0E, 0x0F, 0x0E, 0x0F,
        0x11, 0x11, 0x11, 0x10, 0x10, 0x10, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x0F, 0x0F, 0x0F, 0x0F
    };

    sprite->type = TYPE[spawn_index] << 3;
    sprite->function_holder = OSprites::TRAFFIC_TICK;
}

// Traffic Object has been spawned. Initalize it.
//
// Source: 0x521A
void OTraffic::tick(oentry* sprite)
{
    if (sprite->function_holder == OSprites::TRAFFIC_INIT)
    {
        if (outrun.game_state != GS_INGAME && outrun.game_state != GS_ATTRACT)
        {
            sprite->traffic_proximity = 0;
            move_spawned_sprite(sprite); // Skip collision code
            return;
        }

        sprite->traffic_orig_speed = 0xD4;
        sprite->function_holder = OSprites::TRAFFIC_ENTRY;
    }

    // Skip collision code in first section of level
    if (sprite->function_holder == OSprites::TRAFFIC_ENTRY)
    {
        if (oroad.road_pos >> 16 >= 0x80)
            sprite->function_holder = OSprites::TRAFFIC_TICK;
        else
            move_spawned_sprite(sprite); // Skip collision code
    }

    if (sprite->function_holder == OSprites::TRAFFIC_TICK)
        tick_spawned_sprite(sprite);
}

// Check Traffic Collision
//
// Source: 0x4DAA
void OTraffic::tick_spawned_sprite(oentry* sprite)
{
    if (outrun.tick_frame)
    {
        // Force side of road when in bonus mode, or road splitting
        if (bonus_lhs)
            sprite->control |= OSprites::TRAFFIC_RHS;
        else if (traffic_split)
            sprite->control ^= OSprites::TRAFFIC_RHS;
    
        // Check for collision with player's car
        check_collision(sprite);

        // Calculate X Difference Between Player Car & Traffic.
        // Set Relevant Bits To Denote which side player's car is on in relation to traffic

        if (sprite->z >> 16 <= 0x100) // Value was 0xA0 on original romset and changed for Rev. A
        {
            move_spawned_sprite(sprite);
            return;
        }

        int16_t x_diff = sprite->xw1 + oinitengine.car_x_pos - (oroad.road_width >> 16); // d1
        int16_t x_diff_abs = x_diff < 0 ? -x_diff : x_diff; // d0

        if (x_diff_abs >= 0xA0)
        {
            move_spawned_sprite(sprite);
            return;
        }
        if (x_diff >= 0)
            sprite->traffic_proximity |= BIT_1;
        else
            sprite->traffic_proximity |= BIT_0;

        // Code in block below was added in Rev A. Romset
        if (sprite->xw1 == 0x70)
            sprite->traffic_proximity |= BIT_0;
        else if (sprite->xw1 == -0x70)
            sprite->traffic_proximity |= BIT_1;
        // End Added block
    
        ai_traffic |= sprite->traffic_proximity;
    }

    move_spawned_sprite(sprite);
}

// 0x4E3E
void OTraffic::move_spawned_sprite(oentry* sprite)
{
    // Road Splitting: Return if enemy on opposite side of road to split
    if (oinitengine.road_remove_split)
    {
        if (((oinitengine.route_selected ^ sprite->control) & OSprites::TRAFFIC_RHS) == 0) 
            return;
    }

    if (outrun.game_state != GS_INGAME && outrun.game_state != GS_BONUS && outrun.game_state != GS_ATTRACT)
    {
        osprites.do_spr_order_shadows(sprite);
        return;
    }

    if (outrun.tick_frame)
    {
        // Check closeness bits and setup approproiate lane movement plan for traffic.
        uint8_t traffic_proximity = sprite->traffic_proximity & 3;

        // Other Traffic Close
        if (traffic_proximity)
        {
            // Value transformed as follows:
            // 3 = 0, 2 = 1, 1 = 2
            traffic_proximity ^= 3;
        
            // use_traffic_speed:
            // Sprite hemmed in on left + right. Resort to average traffic speed.
            if (!traffic_proximity)
            {
                sprite->traffic_speed = sprite->traffic_near_speed < 0x70 ? 0x70 : sprite->traffic_near_speed;
                update_props(sprite);
                return;
            }
            // try_move_right
            else if (traffic_proximity & BIT_0)
            {
                if (sprite->xw2 <= 0)
                    sprite->xw2 += 0x70;
            }
            // Try Moving Sprite Left
            else
            {
                if (sprite->xw2 >= 0)
                    sprite->xw2 -= 0x70;
            }
        }
        // not_close:
        // Gradually restore traffic back to original speed. (Routine from 0x50BC rolled in)
        else
        {
            int16_t speed = sprite->traffic_orig_speed - sprite->traffic_speed;
            if (speed > 2) speed = 2;
            else if (speed < -2) speed = -2;
            sprite->traffic_speed += speed;
        }

    }
    // try_lane_change:
    int16_t x_diff = sprite->xw2 - sprite->xw1;

    if (x_diff)
    {
        if (x_diff > 0)
        {
            if ((sprite->traffic_proximity & BIT_0) == 0) // Move Left if no traffic on LHS
                sprite->xw1 += FRAMES_PER_SECOND == 30 ? 4 : 2;
        }
        else if (x_diff < 0)
        {
            if ((sprite->traffic_proximity & BIT_1) == 0) // Move Right if no traffic on RHS
                sprite->xw1 -= FRAMES_PER_SECOND == 30 ? 4 : 2;
        }
    }
    // skip_lane_change:
    update_props(sprite);
}

// skip_lane_change:
// Source: 0x4F0C
void OTraffic::update_props(oentry* sprite)
{
    int32_t z_adjust = (((oinitengine.car_increment >> 16) - sprite->traffic_speed) * (sprite->z >> 16)) << 5;

    if (FRAMES_PER_SECOND == 60)
        z_adjust >>= 1;
    else if (FRAMES_PER_SECOND == 120)
        z_adjust >>= 2;

    sprite->z += z_adjust;
    
    int16_t z16 = sprite->z >> 16;

    // Disable Traffic Object
    if (z16 <= 0)
    {
        olevelobjs.hide_sprite(sprite);
        return;
    }
    // Overtake Traffic Object
    if (z16 >= 0x200)
    {
        osoundint.queue_sound(sound::RESET);
        if (outrun.game_state == GS_INGAME)
        {
            // Update score on overtake
            ostats.update_score(0x20000);
        }

        olevelobjs.hide_sprite(sprite);
        return;
    }

    sprite->priority = sprite->road_priority = z16;

    // Set Screen Y
    sprite->y = -(oroad.road_y[oroad.road_p0 + z16] >> 4) + 223;
    set_zoom_lookup(sprite);

    // Set Screen X
    int16_t* road_x = (sprite->control & OSprites::TRAFFIC_RHS) ? oroad.road1_h : oroad.road0_h;
    int32_t x = (sprite->xw1 * z16) >> 9;
    sprite->x = x + road_x[z16];

    if (z16 <= 8)
    {
        osprites.map_palette(sprite);
        traffic_speed_total += sprite->traffic_speed;
        osprites.do_spr_order_shadows(sprite);
        return;
    }

    // Calculate change in road y, so we can determine incline frame for traffic
    int16_t y = oroad.road_y[oroad.road_p0 - (0x10 / 2)] - oroad.road_y[oroad.road_p0];

    // 0 = No Incline, 10 = Flat Road/Incline
    int8_t incline = (y >= 0x12) ? 0x10 : 0; // d1

    // ------------------------------------------------------------------------
    // Cap Player X Position 
    // Set Horizontal Flip Depending On Position Of Car In Relation To Player
    // ------------------------------------------------------------------------

    x = oinitengine.car_x_pos - (oroad.road_width >> 16);

    if (sprite->control & OSprites::TRAFFIC_RHS)
    {
        x += (oroad.road_width >> 16) << 1;
    }

    x += (oroad.road_x[z16] - oroad.road_x[z16 - (0x10 / 2)]);

    if (x > 0x190) 
        x = 0x190;
    else if (x < -0x190) 
        x = -0x190;

    x = (x >> 2) + (sprite->xw1 >> 2);
    
    int8_t traffic_frame = 0;
    int32_t xabs = x < 0 ? -x : x;

    if (xabs < 0x10)
        traffic_frame = 1;
    else if (xabs < 0x30)
        traffic_frame = 2;
    else
        traffic_frame = 3;

    if (x < 0)
    {
        sprite->control &= ~OSprites::HFLIP;
    }
    else
    {
        sprite->control |= OSprites::HFLIP;
    }

    // ------------------------------------------------------------------------
    // Set palette, sprite data etc. based on:
    // 1/ Traffic Type
    // 2/ Uphill/Straight Road Section
    // 3/ Position of Car in relation to player on x axis
    // ------------------------------------------------------------------------
    
    sprite->pal_src = roms.rom0.read8(TRAFFIC_PROPS + sprite->type + 4) + traffic_pal_cycle;

    int16_t traffic_type = (roms.rom0.read8(TRAFFIC_PROPS + sprite->type + 7) << 5) + (traffic_frame << 2) + incline;
    sprite->addr = roms.rom0.read32(TRAFFIC_DATA + traffic_type);

    osprites.map_palette(sprite);
    traffic_speed_total += sprite->traffic_speed;
    osprites.do_spr_order_shadows(sprite);
}

void OTraffic::set_zoom_lookup(oentry* sprite)
{
    uint16_t road_priority = (sprite->road_priority >> 2) + 4;
    if (road_priority > 0x7F)
        road_priority = 0x7F;

    // Traffic Properties Table
    //
    // +0 [Long] Sprite data address
    // +4 [Byte] Palette
    // +5 [Byte] Collision Mask. Probably to do with the strength/impact of the collision
    // +6 [Byte] Zoom Lookup Value for Width/Height
    // +7 [Byte] Traffic Type

    uint8_t zoom_lookup = roms.rom0.read8(TRAFFIC_PROPS + sprite->type + 6);

    switch (zoom_lookup)
    {
        case 0:
            road_priority += (road_priority >> 3);
            break;
        case 2:
            road_priority += (road_priority >> 2);
            break;
        case 4:
            road_priority += (road_priority >> 1);
            break;
        case 6:
            road_priority += road_priority;
            break;
    }

    sprite->zoom = (uint8_t) road_priority;
}

// Set Maximum number of traffic objects to spawn. 
// Based on difficulty selected and stage number.
//
// Maximum Traffic Per Level
// 
//         | Easy | Norm | Hard | VHar |
//         '------'------'------'------'
//Stage 1  |   2      3      4      5  |
//         '---------------------------'
//Stage 2  |   2      4      5      6  |
//         '---------------------------'
//Stage 3  |   3      5      6      7  |
//         '---------------------------'
//Stage 4  |   4      6      7      8  |
//         '---------------------------'
//Stage 5  |   5      7      8      8  |
//         '---------------------------'
// Source: 0x846E
void OTraffic::set_max_traffic()
{
    const uint8_t MAX_TRAFFIC[] =
    {
    // S1 S2 S3 S4 S5
        2, 2, 3, 4, 5, // Easy Traffic
        3, 4, 5, 6, 7, // Normal Traffic
        4, 5, 6, 7, 8, // Hard Traffic
        5, 6, 7, 8, 8, // Very Hard Traffic
    };

    uint8_t index = (DIP_TRAFFIC * 5) + (oroad.stage_lookup_off / 8);
    max_traffic = MAX_TRAFFIC[index];
}

// -------------
// Traffic Logic
// -------------
//
// 1/ Handles Traffic to Traffic behaviour
// 2/ Adjusts speed of cars to avoid running into each other
// 3/ Sets various sprite bits to denote where traffic is in relation to each other
// 4/ Calculates average speed of all traffic
//
// Notes:
// Processes sprite in hardware ready format and extracts original addresses where necessary.
//
// In use:
//
// d5 = Count of number of traffic sprites spawned
// d7 = Loop counter
//
// a2 = Address of sprite in jump table
// a4 = Address of sprite ready for HW
//
// Source: 0x7990
void OTraffic::traffic_logic()
{
    uint16_t sprite_count = osprites.sprite_count - osprites.spr_cnt_shadow;
    uint16_t spawned = 0; // d5
    
    if (!sprite_count)
    {
        calculate_avg_speed(0);
        return;
    }
       
    oentry* first = 0;
    uint8_t index = 0;
    uint16_t spr_index = osprites.spr_cnt_shadow;

    // Find First Traffic Entry. Note we use the hardware sprite list here to extract the original object.
    for (index = 0; index <= sprite_count; index++)
    {
        uint16_t src_index = osprites.sprite_entries[spr_index++].scratch;

        first = &osprites.jump_table[src_index];
        if (first->control & OSprites::TRAFFIC_SPRITE)
        {
            traffic_adr[spawned++] = first;
            break;
        }
    }

    // No Traffic Found, get out of there
    if (!spawned)
    {
        calculate_avg_speed(0);
        return;
    }

    oentry* next = 0;

    // Compare Current Traffic Entry With Previous One
    for (uint8_t index2 = index + 1; index2 <= sprite_count; index2++)
    {
        uint16_t src_index = osprites.sprite_entries[spr_index++].scratch;
        next = &osprites.jump_table[src_index];
        if (next->control & OSprites::TRAFFIC_SPRITE)
        {
            traffic_adr[spawned++] = next;
            next->traffic_proximity = 0;

            uint16_t z16 = first->z >> 16;

            if (z16 < 0x40)
            {
                first = next;
                continue;
            }

            z16 += (z16 >> 1) + (z16 >> 2); // [x1.75 original value]

            if (z16 <= next->z >> 16)   
            {
                first = next;
                continue;
            }

            next->traffic_proximity |= BIT_2; // Denote entry2 is close to other traffic (z axis)

            int16_t x_diff = first->xw1 - next->xw1; // d1
            int16_t x_diff_abs = x_diff < 0 ? -x_diff : x_diff; // d0

            if (x_diff_abs - 0x80 >= 0)
            {
                first = next;
                continue;
            }

            if (x_diff >= 0)
            {
                first->traffic_proximity |= BIT_1; // Entry 1: Denote traffic on RHS
                next->traffic_proximity |= BIT_0;  // Entry 2: Denote traffic on LHS [remember x scale is reversed on outrun]
            }
            else
            {
                first->traffic_proximity |= BIT_0; // Entry 1: Denote traffic on LHS
                next->traffic_proximity |= BIT_1;  // Entry 2: Denote traffic on RHS
            }

            // Copy car speed into entry 2 to avoid collision
            next->traffic_near_speed = first->traffic_speed;
            first = next;
        }
    }

    calculate_avg_speed(spawned);
}

// Source: 7A6A
void OTraffic::calculate_avg_speed(uint16_t c)
{
    traffic_count = c;

    if (traffic_count != 0)
        traffic_speed_avg = traffic_speed_total / traffic_count;
    traffic_speed_total = 0;
}

// Check For Traffic Collisions
//
// - Check for collision between traffic sprite and player car
// - Setup the skid counter for the player's car
// - Adjust player's speed
//
// Source: 0x50DE
void OTraffic::check_collision(oentry* sprite)
{
    int16_t d0 = 0;

    // Check for collision
    if (sprite->z >> 16 >= 0x1D8)
    {
        int16_t w  = (sprite->width >> 1) + (sprite->width >> 3) + (sprite->width >> 4);
        int16_t x1 = sprite->x - w; // d2
        int16_t x2 = sprite->x + w; // d1

        // Check traffic is directly in front of player's car
        if (x1 < 0 && x2 > 0)
        {
            // Set collision settings from property table
            collision_mask = roms.rom0.read8(TRAFFIC_PROPS + sprite->type + 5);
            d0 = (sprite->x < 0) ? -OCrash::SKID_RESET : OCrash::SKID_RESET;
            d0 += ocrash.skid_counter;

            if (d0 <= OCrash::SKID_MAX && d0 >= -OCrash::SKID_MAX)
                ocrash.skid_counter = d0;

            // Set Ferrari speed based on collision speed
            if (outrun.game_state == GS_ATTRACT || outrun.game_state == GS_INGAME)
            {
                int16_t traffic_speed = sprite->traffic_speed - 80;
                if (traffic_speed < 0) traffic_speed = 0;
                oinitengine.car_increment = (traffic_speed << 16) | (oinitengine.car_increment & 0xFFFF);
                oferrari.car_inc_old = traffic_speed;
                d0 = sound::REBOUND; // rebound sound effect
                collision_traffic++; // denote collision with traffic
            }
        }
    }
    // try_sound:
    uint8_t traffic_fx_old = sprite->traffic_fx;
    sprite->traffic_fx = d0 & 0xFF;

    // New sound effect triggered
    if (!traffic_fx_old && sprite->traffic_fx)
    {
        osoundint.queue_sound(sprite->traffic_fx);
        // Set all proximity bits on
        if (outils::random() & 1)
            sprite->traffic_proximity = 0xFF;
    }
}

// Passing Traffic Sound Effects
// Handle up to four cars passing simulataneously
// Source: 0x7A8C
void OTraffic::traffic_sound()
{
    // Clear traffic data
    osoundint.engine_data[sound::TRAFFIC1] = 0;
    osoundint.engine_data[sound::TRAFFIC2] = 0;
    osoundint.engine_data[sound::TRAFFIC3] = 0;
    osoundint.engine_data[sound::TRAFFIC4] = 0;

    if (outrun.game_state != GS_INGAME && outrun.game_state != GS_ATTRACT)
        return;

    // Return if we have chosen not to play sound in attract mode
    if (outrun.game_state == GS_ATTRACT && !DIP_ADVERTISE)
        return;

    // Return if we haven't spawned any traffic
    if (!traffic_count)
        return;

    // Max number of sounds we can do is 4
    int16_t sounds = traffic_count <= 4 ? traffic_count : 4;

    // Loop through traffic objects that are on screen
    for (int16_t i = sounds - 1; i >= 0; i--)
    {
        oentry* t = traffic_adr[i];
        // Used to set panning of sound as car moves left and right in front of the player
        int16_t pan = t->x >> 5; 
        if (pan < -3) pan = -3;
        if (pan >  3) pan =  3;
        pan &= 7;
        // Position into screen is used to set volume
        uint8_t vol = (t->road_priority & 0x1F0) >> 1;
        osoundint.engine_data[sound::TRAFFIC1 + i] = pan | vol;
        //std::cout << (sound::TRAFFIC1 + i) << std::hex << " pan: " << pan << " vol: " << vol << std::endl;
    }
}