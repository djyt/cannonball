/***************************************************************************
    Level Object Logic
    
    This class handles rendering most of the objects that comprise a typical
    level. 
    
    - Configures rendering properties (co-ordinates, zoom etc.) 
    - Object specific logic, including collision checks & start lights etc.

    The original codebase contains a large amount of code duplication,
    much of which is duplicated here.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "../trackloader.hpp"
#include "engine/outils.hpp"
#include "engine/olevelobjs.hpp"
#include "engine/ostats.hpp"

OLevelObjs olevelobjs;

OLevelObjs::OLevelObjs(void)
{
}

OLevelObjs::~OLevelObjs(void)
{
}

// SetupSpriteEntries
// Initialize default sprite entries for game engine.
// Called when title initalizes.
//
// Once setup, this routine is replaced with SpriteControl
//
// Source: 0x3B48
void OLevelObjs::init_startline_sprites()
{
    // Return if Music Selection Screen
    if (outrun.game_state == GS_MUSIC) return; 
    init_entries(outrun.adr.sprite_def_props1, 0, DEF_SPRITE_ENTRIES);
}

void OLevelObjs::init_timetrial_sprites()
{
    const uint8_t start_index = outrun.ttrial.level == 0 ? 0 : 18;
    init_entries(outrun.adr.sprite_def_props1 + (start_index * 0x10), start_index, DEF_SPRITE_ENTRIES);
}

// Setup hi-score screen sprite entries
void OLevelObjs::init_hiscore_sprites()
{
    init_entries(outrun.adr.sprite_def_props2, 0, HISCORE_SPRITE_ENTRIES);
}

void OLevelObjs::init_entries(uint32_t a4, const uint8_t start_index, const uint8_t no_entries)
{
    // next_sprite:
    for (uint8_t i = start_index; i <= no_entries; i++)
    {
        oentry *sprite = &osprites.jump_table[i];

        sprite->control    = roms.rom0p->read8(&a4);
        sprite->draw_props = roms.rom0p->read8(&a4);
        sprite->shadow     = roms.rom0p->read8(&a4);
        sprite->pal_src    = roms.rom0p->read8(&a4);
        sprite->type       = roms.rom0p->read16(&a4);
        sprite->addr       = roms.rom0p->read32(outrun.adr.sprite_type_table + sprite->type);
        sprite->xw1        = 
        sprite->xw2        = roms.rom0p->read16(&a4);
        sprite->yw         = roms.rom0p->read16(&a4);

        uint16_t z_orig    = roms.rom0p->read16(&a4);

        uint32_t z = z_orig;
        outils::swap32(z);
        sprite->z = z;

        int16_t road_x = oroad.road0_h[z_orig];
        int16_t xw1 = sprite->xw1;

        if (xw1 >= 0 && (sprite->control & OSprites::WIDE_ROAD) == 0)
        {
            xw1 += (oroad.road_width << 1) << 16;
        }

        // on_lhs
        int16_t multiply = (xw1 * z_orig) >> 9; // only used as a 16 bit value so truncate here
        sprite->x = road_x + multiply;

        a4 += 4; // Throw away

        // Hack to choose correct routine, and not use lookup table from ROM
        if (i >= 0 && i <= 27)
            sprite->function_holder = 0; // SpriteCollisionZ1
        else if (i >= 28 && i <= 43)
            sprite->function_holder = 7; // SpriteCollisionZ1C
        else if (no_entries == HISCORE_SPRITE_ENTRIES)
            sprite->function_holder = 8; // SpriteNoCollisionZ2;
        // Vertical Sign on LHS with Lights
        else if (i == 44)
            sprite->function_holder = 4; // Lights
        // Vertical Sign on RHS
        else if (i == 45)
            sprite->function_holder = 0; // SpriteCollisionZ1
        // Two halves of start sign
        else if (i == 46 || i == 47)
            sprite->function_holder = 0; // SpriteNoCollisionZ1
        // Crowd of people
        else if (i >= 48 && i <= 67)
            sprite->function_holder = 8; // SpriteNoCollisionZ2;

        osprites.map_palette(sprite);    
    }
}

// Setup Sprites
//
// Source: 3CB2
//
// Input: Default Zoom Value
void OLevelObjs::setup_sprites(uint32_t z)
{
    // Setup entries that have not yet been enabled
    for (uint8_t i = 0; i < osprites.no_sprites; i++)
    {
        if ((osprites.jump_table[i].control & OSprites::ENABLE) == 0)
        {
            setup_sprite(&osprites.jump_table[i], z);
            return;
        }
    }
    //std::cout << "Need another entry" << std::endl;
}

// Setup Sprite from ROM format for use in game
//
// Source: 3CDE
//
// ROM Setup:
//
// +0: [Byte] Bit 0 = H-Flip Sprite
//            Bit 1 = Enable Shadows
//            
//            Bits 4-7 = Routine Draw Number
// +1: [Byte] Sprite X World Position
// +2: [Word] Sprite Y World Position
// +5: [Byte] Sprite Type
// +7: [Byte] Sprite Palette
void OLevelObjs::setup_sprite(oentry* sprite, uint32_t z)
{
    #define READ8(x)  trackloader.read8(trackloader.scenerymap_data, x)
    #define READ16(x) trackloader.read16(trackloader.scenerymap_data, x)
    #define READ32(x) trackloader.read32(trackloader.scenerymap_data, x)

    sprite->control |= OSprites::ENABLE; // Turn sprite on
    uint32_t addr = osprites.seg_spr_addr + osprites.seg_spr_offset1;

    // Set sprite x,y (world coordinates)
    sprite->xw1     = 
    sprite->xw2     = READ8(addr + 1) << 4;
    sprite->yw      = READ16(addr + 2) << 7;
    sprite->type    = ((uint8_t) READ8 (addr + 5)) << 2;
    sprite->addr    = roms.rom0p->read32(outrun.adr.sprite_type_table + sprite->type);

    sprite->pal_src = READ8 (addr + 7);
    
    osprites.map_palette(sprite);

    sprite->width = 0;
    sprite->reload = 0;
    sprite->z = z; // Set default zoom
    
    if (READ8(addr + 0) & 1)
        sprite->control |= OSprites::HFLIP;
    else
        sprite->control &=~ OSprites::HFLIP;

    if (READ8(addr + 0) & 2)
        sprite->control |= OSprites::SHADOW;
    else
        sprite->control &=~ OSprites::SHADOW;

    if ((int16_t) (oroad.road_width >> 16) > 0x118)
        sprite->control |= OSprites::WIDE_ROAD;
    else
        sprite->control &=~ OSprites::WIDE_ROAD;

    sprite->draw_props = READ8(addr + 0) & 0xF0;
    sprite->function_holder = sprite->draw_props >> 4; // set sprite type

    setup_sprite_routine(sprite);
}

void OLevelObjs::setup_sprite_routine(oentry* sprite)
{
    switch (sprite->function_holder)
    {
        // Normal Sprite: (Possible With/Without Collision)
        case 0:
            sprite->shadow = 7;
            sprite->draw_props |= 8; // x = anchor centre, y = anchor bottom
            break;
        
        case 1: // Grass Sprite
        case 11:  // Stone Strips
            sprite->shadow = 7;
            if (sprite->control & OSprites::HFLIP)
                sprite->draw_props |= 2;
            else
                sprite->draw_props |= 1;
            break;

        // Overhead Clouds
        case 2: 
            sprite->shadow = 3;
            if (sprite->control & OSprites::HFLIP)
                sprite->draw_props |= 0xA;
            else
                sprite->draw_props |= 9;

            if (sprite->draw_props & BIT_0)
                sprite->reload = -0x20;
            else
                sprite->reload = 0x20;

            sprite->xw2 = 0;
            break;

        // Water Sprite
        case 3:
            sprite->shadow = 3;
            if (sprite->control & OSprites::HFLIP)
                sprite->draw_props |= 2; // anchor x right
            else
                sprite->draw_props |= 1; // anchor x left
            break;
        
        case 4: // Sprite Lights
        case 5: // Checkpoint Bottom
        case 6: // Checkpoint Top
        case 8: // No Collision Check
        case 9: // Wide Rocks (Stage 2)
            sprite->shadow = 7;
            sprite->draw_props |= 8; // x = anchor centre, y = anchor bottom
            break;
        
        // Draw From Top Left Collision Check
        case 7:
            sprite->shadow = 7;
            if (sprite->control & OSprites::HFLIP)
                sprite->draw_props |= 9;
            else
                sprite->draw_props |= 0xA;
            break;

        // Sand Strips
        case 10:
        case 14: // version for wider road widths
            sprite->shadow = 3;
            if (sprite->control & OSprites::HFLIP)
                sprite->draw_props |= 2;
            else
                sprite->draw_props |= 1;
            break;
    
        // Mini Tree
        case 12:
            sprite->shadow = 7;
            if (sprite->control & OSprites::HFLIP)
                sprite->draw_props |= 0xA;
            else
                sprite->draw_props |= 9;
            break;

        // Spray
        case 13:
            sprite->shadow = 3;
            sprite->draw_props |= 0;
            break;
    }
}

void OLevelObjs::do_sprite_routine()
{
    for (uint8_t i = 0; i < osprites.no_sprites; i++)
    {
        oentry* sprite = &osprites.jump_table[i];

        if (sprite->control & OSprites::ENABLE)
        {
            switch (sprite->function_holder)
            {
                // Normal Sprite: (Possible With/Without Collision, Zoom 1)
                case 0:
                    if (sprite->yw == 0)
                       sprite_normal(sprite, 1);
                    else
                       set_spr_zoom_priority(sprite, 1);
                    break;

                // Grass Sprite
                case 1:
                    sprite_grass(sprite);
                    break;

                // Sprite based clouds that span entire sky
                case 2:
                    sprite_clouds(sprite);
                    break;

                // Water on LHS of Stage 1
                case 3:
                    sprite_water(sprite);
                    break;

                // Start Lights & Base Pillar of Checkpoint Sign
                case 4:
                    sprite_lights(sprite);
                    break;
                
                // 5 - Checkpoint (Bottom Of Sign)
                case 5:
                    set_spr_zoom_priority(sprite, 1);
                    break;

                // 6 - Checkpoint (Top Of Sign)
                case 6:
                    set_spr_zoom_priority(sprite, 1);
                    // Have we passed the checkpoint?
                    if (!(sprite->control & OSprites::ENABLE))
                        oinitengine.checkpoint_marker = -1;
                    break;

                // Draw From Centre Collision Check
                case 7:
                    sprite_collision_z1c(sprite);
                    break;

                // Normal Sprite: (Collision, Zoom 2)
                case 8:
                    sprite_normal(sprite, 2);
                    break;

                // Wide Rocks on Stage 2
                case 9:
                    sprite_rocks(sprite);
                    break;

                // Sand Strips
                case 10:
                    do_thickness_sprite(sprite, outrun.adr.sprite_sand);
                    break;

                // Stone Strips
                case 11:
                    do_thickness_sprite(sprite, outrun.adr.sprite_stone);
                    break;

                // Mini-Tree (Stage 5, Level ID: 0x24)
                case 12:
                    sprite_minitree(sprite);
                    break;
                
                // Track Debris on Stage 3a
                case 13:
                    sprite_debris(sprite);
                    break;

                // Sand (Again) - Used in end sequence #2
                case 14:
                    do_thickness_sprite(sprite, outrun.adr.sprite_sand);
                    break;

                /*default:
                    std::cout << "do_sprite_routine() " << int16_t(sprite->function_holder) << std::endl;
                    break;*/
            }
        }
    }
}

// Source: 4048
void OLevelObjs::sprite_normal(oentry *sprite, uint8_t zoom)
{
    // Omit collision check if we're already colliding with something
    if (sprite_collision_counter != 0 || (sprite->z >> 16) < 0x1B0)
    {
        set_spr_zoom_priority(sprite, zoom);
        return;
    }
    // Check Collision with sprite

    // First read collision offsets from table
    uint32_t offset_addr = SPRITE_X_OFFS + sprite->type;
    int16_t x1;
    int16_t x2; 

    // H-Flip - swap x co-ordinates
    if (sprite->control & OSprites::HFLIP)
    {
        x2 = (int16_t) roms.rom0.read16(&offset_addr);
        x1 = (int16_t) roms.rom0.read16(&offset_addr);
        x2 = -x2;
        x1 = -x1;
    }
    else
    {
        x1 = (int16_t) roms.rom0.read16(&offset_addr);
        x2 = (int16_t) roms.rom0.read16(&offset_addr);
    }

    // If off left hand side or off right hand side of screen
    if (sprite->x + x1 > 0 || sprite->x + x2 < 0)
    {
        set_spr_zoom_priority(sprite, zoom);
        return;
    }

    collision_sprite++; // Denote collision with sprite
    sprite_collision_counter = COLLISION_RESET; // Reset sprite collision counter
    set_spr_zoom_priority(sprite, zoom);
}

// Identical to sprite_normal, but calls the countdown routine at the end. 
//
// Source: 0x4658
void OLevelObjs::sprite_lights(oentry *sprite)
{
    // Omit collision check if we're already colliding with something
    if (sprite_collision_counter != 0 || (sprite->z >> 16) < 0x1B0)
    {
        sprite_lights_countdown(sprite);
        return;
    }
    // Check Collision with sprite

    // First read collision offsets from table
    uint32_t offset_addr = SPRITE_X_OFFS + sprite->type;
    int16_t x1;
    int16_t x2; 

    // H-Flip - swap x co-ordinates
    if (sprite->control & OSprites::HFLIP)
    {
        x2 = (int16_t) roms.rom0.read16(&offset_addr);
        x1 = (int16_t) roms.rom0.read16(&offset_addr);
        x2 = -x2;
        x1 = -x1;
    }
    else
    {
        x1 = (int16_t) roms.rom0.read16(&offset_addr);
        x2 = (int16_t) roms.rom0.read16(&offset_addr);
    }

    // If off left hand side or off right hand side of screen
    if (sprite->x + x1 > 0 || sprite->x + x2 < 0)
    {
        sprite_lights_countdown(sprite);
        return;
    }

    collision_sprite++; // Denote collision with sprite
    sprite_collision_counter = COLLISION_RESET; // Reset sprite collision counter
    sprite_lights_countdown(sprite);
}

void OLevelObjs::sprite_lights_countdown(oentry *sprite)
{
    // Yes, here we use the game_state to control the countdown palette :-s
    int8_t countdown_pal = outrun.game_state - 9;

    // We're in countdown mode, let's adjust the palette
    if (countdown_pal >= 0 && countdown_pal <= 3)
    {
        countdown_pal += 0x7A + ostats.cur_stage;
        sprite->pal_src = countdown_pal;
        osprites.map_palette(sprite);
    }
    
    set_spr_zoom_priority(sprite, 1); // WIDE_ROAD must be set for this to work.
}

// Set Sprite Priority To Other Sprites
// Set Sprite Priority To Road
// Set Index To Lookup Sprite Settings (Width/Height) From Zoom Value
//
// Source Address: 0x404A
void OLevelObjs::set_spr_zoom_priority(oentry *sprite, uint8_t zoom)
{
    osprites.move_sprite(sprite, 0);
    uint16_t z16 = sprite->z >> 16;

    if (z16 < 4) return;
    if (z16 >= 0x200)
    {
        hide_sprite(sprite);
        return;
    }
    sprite->road_priority = z16;
    sprite->priority = z16;        
    sprite->zoom = z16 >> zoom;

    // Set Sprite Y Position [SCREEN]
    // 1/ Use Y Offset From Road Position [Screen]
    // 2/ Use Sprite Y World Data if not 0, converted to a screen position [World]

    int32_t road_y = -(oroad.road_y[oroad.road_p0 + z16] >> 4) + 223;

    if (sprite->yw != 0)
    {
        uint32_t yw = sprite->yw * z16; // Note the final product is a LONG, not a word here
        outils::swap32(yw);
        outils::sub16(yw, road_y);
    }
    sprite->y = road_y;

    //    Set Sprite X Position [SCREEN]
    //    1/ Use X Offset From Road Position [Screen]
    //    2/ Use Sprite X World Data
    
    int16_t road_x = oroad.road0_h[z16];
    int16_t xw1 = sprite->xw1;
   
    if (xw1 >= 0)
    {
        // Bit of a hack here to avoid code duplication
        if (sprite->function_holder >= 4 && sprite->function_holder <= 6) 
            xw1 += (oroad.road_width >> 16) << 1;
        else
        {
            if ((sprite->control & OSprites::WIDE_ROAD) == 0)
                xw1 += (oroad.road_width >> 16) << 1;
        }
    }

    int32_t multiply = (xw1 * z16) >> 9; // only used as a 16 bit value so truncate here
    sprite->x = road_x + multiply;

    osprites.do_spr_order_shadows(sprite);
}

// Seems to be identical to sprite_normal
//
// Source: 0x4828
// 
// - Zoom Shift: 1
// - Test For Collision
void OLevelObjs::sprite_collision_z1c(oentry* sprite)
{
    // Omit collision check if we're already colliding with something
    if (sprite_collision_counter != 0 || (sprite->z >> 16) < 0x1B0)
    {
        set_spr_zoom_priority2(sprite, 1);
        return;
    }
    // Check Collision with sprite

    // First read collision offsets from table
    uint32_t offset_addr = SPRITE_X_OFFS + sprite->type;
    int16_t x1;
    int16_t x2; 

    // H-Flip - swap x co-ordinates
    if (sprite-> control & OSprites::HFLIP)
    {
        x2 = (int16_t) roms.rom0.read16(&offset_addr);
        x1 = (int16_t) roms.rom0.read16(&offset_addr);
        x2 = -x2;
        x1 = -x1;
    }
    else
    {
        x1 = (int16_t) roms.rom0.read16(&offset_addr);
        x2 = (int16_t) roms.rom0.read16(&offset_addr);
    }

    int16_t centre = (x2 - x1) >> 1; // d0
    x1 -= centre;
    x2 -= centre;

    // If off left hand side or off right hand side of screen
    if (sprite->x + x1 > 0 || sprite->x + x2 < 0)
    {
        set_spr_zoom_priority2(sprite, 1);
        return;
    }

    collision_sprite++; // Denote collision with sprite
    sprite_collision_counter = COLLISION_RESET; // Reset sprite collision counter
    set_spr_zoom_priority2(sprite, 1);
}

// Almost identical to set_spr_zoom_priority
// 
// Differences highlighted below. 
// 
// Set Sprite Priority To Other Sprites
// Set Sprite Priority To Road
// Set Index To Lookup Sprite Settings (Width/Height) From Zoom Value
//
// Source Address: 0x404A
void OLevelObjs::set_spr_zoom_priority2(oentry *sprite, uint8_t zoom)
{
    osprites.move_sprite(sprite, 0);
    uint16_t z16 = sprite->z >> 16;

    if (z16 < 4) return;
    if (z16 >= 0x200)
    {
        hide_sprite(sprite);
        return;
    }
    sprite->road_priority = z16;
    sprite->priority = z16;    
    sprite->zoom = z16 >> zoom;

    // Code differs from below

    // Set Sprite X Position [SCREEN]
    // 1/ Use X Offset From Road Position [Screen]
    // 2/ Use Sprite X World Data

    int16_t road_x = oroad.road0_h[z16];
    int16_t xw1 = sprite->xw1;

    if (xw1 >= 0 && (sprite->control & OSprites::WIDE_ROAD) == 0)
    {
        xw1 +=  ((int16_t) (oroad.road_width >> 16)) << 1;
    }

    int32_t multiply = (xw1 * z16) >> 9; // only used as a 16 bit value so truncate here
    road_x += multiply;
    if (road_x > (160 + config.s16_x_off) || road_x < -(160 + config.s16_x_off)) return; // NEW LINE compared with original routine (added for ROM REV. A)
    sprite->x = road_x;

    // Set Sprite Y Position [SCREEN]
    // 1/ Use Y Offset From Road Position [Screen]
    // 2/ Use Sprite Y World Data if not 0, converted to a screen position [World]
    sprite->y = -(oroad.road_y[oroad.road_p0 + z16] >> 4) + 223;

    osprites.do_spr_order_shadows(sprite);
}

// Water, as used on LHS of Stage 1
//
// Source: 0x4408
void OLevelObjs::sprite_water(oentry* sprite)
{
    // check_spray
    if (spray_counter == 0)
    {
        if ((sprite->z >> 16) < 0x1B0)
        {

        }
        // Check whether to initialise spray
        else if (((sprite->control & OSprites::HFLIP) == 0               && sprite->x < 0) ||
                 ((sprite->control & OSprites::HFLIP) == OSprites::HFLIP && sprite->x > 0))
        {
            spray_counter = SPRAY_RESET;
            spray_type = 0;
        }
    }
    do_thickness_sprite(sprite, outrun.adr.sprite_water);
}

// Grass Sprites
//
// Source: 0x4416
void OLevelObjs::sprite_grass(oentry* sprite)
{
    // check_spray
    if (spray_counter == 0)
    {
        if ((sprite->z >> 16) < 0x1B0)
        {

        }
        // Check whether to initialise spray
        else if (((sprite->control & OSprites::HFLIP) == 0               && sprite->x < 0) ||
                 ((sprite->control & OSprites::HFLIP) == OSprites::HFLIP && sprite->x > 0))
        {
            spray_counter = SPRAY_RESET;
            spray_type = 4; // Set Spray Type = Yellow
            if (sprite->pal_src != 0x49) // Check whether palette is set to green/grass palette
                spray_type = 8; // Set Spray Type = Green
        }
    }
    do_thickness_sprite(sprite, outrun.adr.sprite_grass);
}

// Sprite: MiniTrees 
//
// - Rows of short tree/shrubs found on Stage 5
// - Frame Changes Based On Current Y Position Of Sprite
// 
// Note: This routine is very similar to do_thickness_sprites and can probably be refactored
//
// Source: 0x428A

void OLevelObjs::sprite_minitree(oentry* sprite)
{
    osprites.move_sprite(sprite, 0);
    uint16_t z16 = sprite->z >> 16;

    if (z16 < 4) return;
    if (z16 >= 0x200)
    {
        hide_sprite(sprite);
        return;
    }
     
    sprite->road_priority = z16;
    sprite->priority = z16;    

    // 44c4
    int16_t road_x = oroad.road0_h[z16];
    int16_t xw1 = sprite->xw1;

    if (xw1 >= 0)
    {
        xw1 += ((int16_t) (oroad.road_width >> 16)) << 1;
    }

    int32_t multiply = (xw1 * z16) >> 9;
    road_x += multiply;

    if (road_x >= (160 + config.s16_x_off) || road_x < -(160 + config.s16_x_off)) return;
    sprite->x = road_x;

    // 44fc
    int32_t road_y = -(oroad.road_y[oroad.road_p0 + z16] >> 4) + 223;
    sprite->y = road_y; // Set Sprite Y (Screen/Camera)

    uint16_t z = z16 >> 1;

    if (z >= 0x80) 
    {
        // don't choose a custom frame
        sprite->zoom = (uint8_t) z; // Set Entry Number For Zoom Lookup Table
        sprite->addr = roms.rom0p->read32(outrun.adr.sprite_minitree); // Set to first frame in table
    }
    // Use Table to alter sprite based on its y position.
    //
    // Input = Y Position
    //
    // Format:
    // +0: Frame Number To Use
    // +2: Entry In Zoom Lookup Table
    else
    {
        z <<= 1; // Note we can't use original z16, so don't try to optimize this
        uint8_t offset = roms.rom0.read8(MAP_Y_TO_FRAME + z);
        sprite->zoom = roms.rom0.read8(MAP_Y_TO_FRAME + z + 1);
        sprite->addr = roms.rom0p->read32(outrun.adr.sprite_minitree + offset);
    }
    // order_sprites
    osprites.do_spr_order_shadows(sprite);
}

// Source: 
void OLevelObjs::sprite_debris(oentry* sprite)
{
    const uint8_t zoom = 2;
    if (spray_counter != 0 || (sprite->z >> 16) < 0x1B0)
    {
        set_spr_zoom_priority(sprite, zoom);
        return;
    }
    // Check Collision with sprite

    // First read collision offsets from table
    uint32_t offset_addr = SPRITE_X_OFFS + sprite->type;
    int16_t x1;
    int16_t x2; 

    // H-Flip - swap x co-ordinates
    if (sprite->control & OSprites::HFLIP)
    {
        x2 = (int16_t) roms.rom0.read16(&offset_addr);
        x1 = (int16_t) roms.rom0.read16(&offset_addr);
        x2 = -x2;
        x1 = -x1;
    }
    else
    {
        x1 = (int16_t) roms.rom0.read16(&offset_addr);
        x2 = (int16_t) roms.rom0.read16(&offset_addr);
    }

    // If off left hand side or off right hand side of screen
    if (sprite->x + x1 > 0 || sprite->x + x2 < 0)
    {
        set_spr_zoom_priority(sprite, zoom);
        return;
    }

    // Passing through spray!
    spray_counter = SPRAY_RESET;
    spray_type = 0xC;
    set_spr_zoom_priority(sprite, zoom);
}

// Sprite based clouds that span entire sky.
//
// Appear on Stage 3 - Rightmost Route.
//
// Source: 0x4144
void OLevelObjs::sprite_clouds(oentry* sprite)
{
    osprites.move_sprite(sprite, 1);
    uint16_t z16 = sprite->z >> 16;

    if (z16 < 4)
    {
        int32_t road_x = oroad.road0_h[z16];
        sprite->type = road_x;
        return;
    }
    if (z16 >= 0x200)
    {
        hide_sprite(sprite);
        return;
    }
     
    sprite->road_priority = z16;
    sprite->priority      = z16;
    sprite->y = -((z16 * oroad.horizon_y2) >> 9) + oroad.horizon_y2; // muls (two 16 bit values, 32 bit result)

    uint16_t type = sprite->type; // d1
    int16_t road_x = oroad.road0_h[z16]; // d0
    sprite->type = road_x;
    road_x -= type;
    type = (z16 >> 2);
    
    if (type != 0)
    {
        //41b6
        road_x += sprite->xw2;
        if (road_x >= 0)
        {
            // 41be
            do
            {
                road_x -= type;
            }
            while (road_x >= 0);
            road_x += type;
        }
        else
        {
            // 41c8
            do
            {
                road_x += type;
            }
            while (road_x < 0);
            road_x -= type;
        }
        // 41ce set_sprite_x_world
        sprite->xw2 = road_x;
    }

    // 41d2 - reload is used as an x-offset here, not as a reload value
    sprite->x = sprite->xw2 + sprite->reload;
    sprite->pal_src = 0xCD;

    uint16_t z = z16 >> 1;

    if (z >= 0x80)
    {
        // 421c
        sprite->zoom = (uint8_t) z; // Set Entry Number For Zoom Lookup Table
        sprite->addr = roms.rom0p->read32(outrun.adr.sprite_cloud);
    }
    else
    {
        // 41f8
        z <<= 1;
        uint8_t lookup_z = roms.rom0.read8(MOVEMENT_LOOKUP_Z + z);
        sprite->addr = roms.rom0p->read32(outrun.adr.sprite_cloud + lookup_z);
        sprite->zoom = roms.rom0.read8(MOVEMENT_LOOKUP_Z + z + 1);
    }
    // end
    osprites.map_palette(sprite);
    osprites.do_spr_order_shadows(sprite);  
}

void OLevelObjs::do_thickness_sprite(oentry* sprite, const uint32_t sprite_table_address)
{
    osprites.move_sprite(sprite, 0);
    uint16_t z16 = sprite->z >> 16;

    if (z16 < 4) return;
    if (z16 >= 0x200)
    {
        hide_sprite(sprite);
        return;
    }
     
    sprite->road_priority = z16;
    sprite->priority = z16;    

    // 44c4
    int16_t road_x = oroad.road0_h[z16];
    int16_t xw1 = sprite->xw1;

    if (xw1 >= 0)
    {
        if (sprite->id != 14 || (sprite->control & OSprites::WIDE_ROAD) == 0) // Rolled in separate routine with this one line. (Sand 2 used in bonus sequence)
            xw1 += ((int16_t) (oroad.road_width >> 16)) << 1;
    }

    int32_t multiply = (xw1 * z16) >> 9;
    road_x += multiply;

    if (road_x >= (160 + config.s16_x_off) || road_x < -(160 + config.s16_x_off)) return;
    sprite->x = road_x;

    // 44fc
    int32_t road_y = -(oroad.road_y[oroad.road_p0 + z16] >> 4) + 223;
    sprite->y = road_y; // Set Sprite Y (Screen/Camera)
    // 4518 - Sprite thickness code

    uint16_t z = z16 >> 1;

    if (z >= 0x80) 
    {
        //use_large_frame (don't choose a custom frame)
        sprite->zoom = (uint8_t) z; // Set Entry Number For Zoom Lookup Table
        sprite->addr = roms.rom0p->read32(0x3C + sprite_table_address); // Set default frame for larger sprite
    }
    else
    {
        // use custom frame for sprite
        sprite->zoom = 0x80; // cap sprite_z minimum to 0x80
        z = (z >> 1) & 0x3C; // Mask over lower 2 bits, so the frame aligns to a word
        sprite->addr = roms.rom0p->read32(z + sprite_table_address); // Set Frame Data Based On Zoom Value
    }
    // order_sprites
    osprites.do_spr_order_shadows(sprite);
}

// ---------------
// SpriteWideRocks
// ---------------
//
// Note doesn't do same adjustment of y position using z 
// Used for rocks on Stage 2 (rightmost route)
//
// Similar routine to SpriteCollisionZ1 (0x4048) but with additional width checks
// 
// TODO: Merge with sprite_normal routine and then pass in the set_spr_zoom_priority_rocks routine as an argument
//
// Source: 0x492A

void OLevelObjs::sprite_rocks(oentry *sprite)
{
    uint8_t zoom = 1;
    // Omit collision check if we're already colliding with something
    if (sprite_collision_counter != 0 || (sprite->z >> 16) < 0x1B0)
    {
        set_spr_zoom_priority_rocks(sprite, zoom);
        return;
    }
    // Check Collision with sprite

    // First read collision offsets from table
    uint32_t offset_addr = SPRITE_X_OFFS + sprite->type;
    int16_t x1;
    int16_t x2; 

    // H-Flip - swap x co-ordinates
    if (sprite->control & OSprites::HFLIP)
    {
        x2 = (int16_t) roms.rom0.read16(&offset_addr);
        x1 = (int16_t) roms.rom0.read16(&offset_addr);
        x2 = -x2;
        x1 = -x1;
    }
    else
    {
        x1 = (int16_t) roms.rom0.read16(&offset_addr);
        x2 = (int16_t) roms.rom0.read16(&offset_addr);
    }

    // If off left hand side or off right hand side of screen
    if (sprite->x + x1 > 0 || sprite->x + x2 < 0)
    {
        set_spr_zoom_priority_rocks(sprite, zoom);
        return;
    }

    collision_sprite++; // Denote collision with sprite
    sprite_collision_counter = COLLISION_RESET; // Reset sprite collision counter
    set_spr_zoom_priority_rocks(sprite, zoom);
}

// Set Sprite Priority To Other Sprites
// Set Sprite Priority To Road
// Set Index To Lookup Sprite Settings (Width/Height) From Zoom Value
//
// Source Address: 0x404A
void OLevelObjs::set_spr_zoom_priority_rocks(oentry *sprite, uint8_t zoom)
{
    osprites.move_sprite(sprite, 0);
    uint16_t z16 = sprite->z >> 16;

    if (z16 < 4) return;
    if (z16 >= 0x200)
    {
        hide_sprite(sprite);
        return;
    }
    sprite->road_priority = z16;
    sprite->priority = z16;        
    sprite->zoom = z16 >> zoom;

    // Set Sprite Y Position [SCREEN]
    // 1/ Use Y Offset From Road Position [Screen]
    // 2/ Use Sprite Y World Data if not 0, converted to a screen position [World]

    int32_t road_y = -(oroad.road_y[oroad.road_p0 + z16] >> 4) + 223;
    sprite->y = road_y;

    //    Set Sprite X Position [SCREEN]
    //    1/ Use X Offset From Road Position [Screen]
    //    2/ Use Sprite X World Data
    
    int16_t road_x = oroad.road0_h[z16];
    int16_t xw1 = sprite->xw1;
   
    if (xw1 >= 0 && (sprite->control & OSprites::WIDE_ROAD) == 0)
    {
        xw1 +=  ((int16_t) (oroad.road_width >> 16)) << 1;
    }

    int32_t multiply = (xw1 * z16) >> 9; // only used as a 16 bit value so truncate here
    sprite->x = road_x + multiply;

    // Additional Draw Check Here, to prevent sprite from wrapping around screen and redrawing on left
    // Remember this routine is setup to draw from centre x

    uint16_t width = (sprite->width >> 1) + 160 + config.s16_x_off;
    if (sprite->x >= width || sprite->x + width < 0) return;

    osprites.do_spr_order_shadows(sprite);
}

// Source Address: 0x4648
void OLevelObjs::hide_sprite(oentry *sprite)
{
    sprite->z = 0;
    sprite->zoom = 0; // Hide the sprite
    sprite->control &= ~OSprites::ENABLE; // Disable entry in jump table
}