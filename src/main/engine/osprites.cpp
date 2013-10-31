/***************************************************************************
    Sprite Handling Routines.
    
    - Initializing Sprites from level data.
    - Mapping palettes to sprites.
    - Ordering sprites by priority.
    - Adding shadows to sprites where appropriate.
    - Clipping sprites based on priority in relation to road hardware.
    - Conversion from internal format to output format required by hardware.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "../trackloader.hpp"

#include "engine/oanimseq.hpp"
#include "engine/ocrash.hpp"
#include "engine/oferrari.hpp"
#include "engine/olevelobjs.hpp"
#include "engine/osprites.hpp"
#include "engine/otraffic.hpp"
#include "engine/ozoom_lookup.hpp"

OSprites osprites;

OSprites::OSprites(void)
{
}

OSprites::~OSprites(void)
{
}

void OSprites::init()
{
    // Set activated number of sprites based on config
    no_sprites = config.engine.level_objects ? SPRITE_ENTRIES : 0x4F;

    // Also handled by clear_palette_data() now
    for (uint16_t i = 0; i < 0x100; i++)
        pal_lookup[i] = 0;

    for (uint16_t i = 0; i < 0x2000; i++)
    {
        sprite_order[i] = 0;
        sprite_order2[i] = 0;
    }

    // Reset hardware entries
    for (uint16_t i = 0; i < JUMP_ENTRIES_TOTAL; i++)
        sprite_entries[i].init();

    for (uint8_t i = 0; i < SPRITE_ENTRIES; i++)
        jump_table[i].init(i);

    // Ferrari + Passenger Sprites
    jump_table[SPRITE_FERRARI].init(SPRITE_FERRARI);        // Ferrari
    jump_table[SPRITE_PASS1].init(SPRITE_PASS1);
    jump_table[SPRITE_PASS2].init(SPRITE_PASS2);
    jump_table[SPRITE_SHADOW].init(SPRITE_SHADOW);          // Shadow Settings
    jump_table[SPRITE_SHADOW].shadow = 7;
    jump_table[SPRITE_SMOKE1].init(SPRITE_SMOKE1);
    jump_table[SPRITE_SMOKE2].init(SPRITE_SMOKE2);

    oferrari.init(&jump_table[SPRITE_FERRARI], &jump_table[SPRITE_PASS1], &jump_table[SPRITE_PASS2], &jump_table[SPRITE_SHADOW]);
    
    // ------------------------------------------------------------------------
    // Traffic in Right Hand Lane At Start of Stage 1
    // ------------------------------------------------------------------------

    for (uint8_t i = SPRITE_TRAFF1; i <= SPRITE_TRAFF8; i++)
    {
        jump_table[i].init(i);      
        jump_table[i].control |= SHADOW;
        jump_table[i].addr = outrun.adr.sprite_porsche; // Initial offset of traffic sprites. Will be changed.
    }

    // ------------------------------------------------------------------------
    // Crash Sprites
    // ------------------------------------------------------------------------

    for (uint8_t i = SPRITE_CRASH; i <= SPRITE_CRASH_PASS2_S; i++)
    {
        jump_table[i].init(i);
    }

    jump_table[SPRITE_CRASH_PASS1].draw_props = oentry::BOTTOM;
    jump_table[SPRITE_CRASH_PASS2].draw_props = oentry::BOTTOM;

    jump_table[SPRITE_CRASH_PASS1_S].shadow     = 7;
    jump_table[SPRITE_CRASH_PASS1_S].addr       = outrun.adr.shadow_data;
    jump_table[SPRITE_CRASH_PASS2_S].shadow     = 7;
    jump_table[SPRITE_CRASH_PASS2_S].draw_props = oentry::BOTTOM;
    jump_table[SPRITE_CRASH_PASS2_S].addr       = outrun.adr.shadow_data;
    
    jump_table[SPRITE_CRASH_SHADOW].shadow     = 7;
    jump_table[SPRITE_CRASH_SHADOW].zoom       = 0x80;
    jump_table[SPRITE_CRASH_SHADOW].draw_props = oentry::BOTTOM;
    jump_table[SPRITE_CRASH_SHADOW].addr       = outrun.adr.shadow_data;

    ocrash.init(
        &jump_table[SPRITE_CRASH], 
        &jump_table[SPRITE_CRASH_SHADOW], 
        &jump_table[SPRITE_CRASH_PASS1],
        &jump_table[SPRITE_CRASH_PASS1_S],
        &jump_table[SPRITE_CRASH_PASS2],
        &jump_table[SPRITE_CRASH_PASS2_S]
    );

    // ------------------------------------------------------------------------
    // Animation Sequence Sprites
    // ------------------------------------------------------------------------

    jump_table[SPRITE_FLAG].init(SPRITE_FLAG);
    oanimseq.init(jump_table);
    
    seg_pos             = 0;
    seg_total_sprites   = 0;
    seg_sprite_freq     = 0;
    seg_spr_offset2     = 0;
    seg_spr_offset1     = 0;
    seg_spr_addr        = 0;

    do_sprite_swap      = false;
    sprite_scroll_speed = 0;
    shadow_offset       = 0;
    sprite_count        = 0;
    spr_cnt_main        = 0;
    spr_cnt_shadow      = 0;

    spr_col_pal         = 0;
    pal_copy_count      = 0;    
}

// Swap Sprite RAM And Update Palette Data
void OSprites::update_sprites()
{
	if (do_sprite_swap)
	{
        do_sprite_swap = false;
        video.sprite_layer->swap();
        copy_palette_data();
	}
}

// Disable All Sprite Entries
// Source: 0x4A50
void OSprites::disable_sprites()
{
    for (uint8_t i = 0; i < SPRITE_ENTRIES; i++)
        jump_table[i].control &= ~OSprites::ENABLE;
}

void OSprites::tick()
{
    sprite_control();
}

// Sprite Control
//
// Source: 3BEE
//
// Notes:
// - Setup Jump Table Entry #2, the sprite control. This in turn is used to control and setup all the sprites.
// - Read 4 Byte Entry From Road_Seg_Adr1 which indicates the upcoming block of sprite data for the level
// - This first block of data specifies the position, total number of sprites in the block we want to try rendering 
//   and appropriate lookup for the sprite number/frequency info.
//
// - This second table in memory specifies the frequency and number of sprites in the sequence. 
//  
// - The second table also contains the actual sprite info (x,y,palette,type). This can be multipled sprites.
//
// ----------------------------------
//
// road_seg_addr1 Format: [4 byte boundaries]
//
// [+0] Road Position [Word]
// [+2] Number Of Sprites In Segment [Byte]
// [+3] Sprite Data Entry Number From Lookup Table * 4 [Byte]
//
// Sprite Master Table Format @ 0x1A43C:
//
// A Table Containing a series of longs. Each address in this table represents:
//
// [+0] Sprite Frequency Value Bitmask [Word]
// [+2] Reload Value For Sprite Info Offset [Word]
// [+4] Start of table with x,y,type,palette etc.
//      This table can appear as many times as desired in each block and follows the format below, starting +4:
//
// -------------------------------------------------
// [+0] [Byte] Bit 0 = H-Flip Sprite
//             Bit 1 = Enable Shadows
//            
//             Bits 4-7 = Routine Draw Number
// [+1] [Byte] Sprite X World Position
// [+2] [Word] Sprite Y World Position
// [+5] [Byte] Sprite Type
// [+7] [Byte] Sprite Palette
//-------------------------------------------------
//
// The Frequency bitmask, for example 111000111 00111000
// rotates left, and whenever a bit is set, a sprite from the sequence is rendered.
//
// When a bit is unset no draw occurs on that call.
//
// The entire sequence can repeat, until the max sprites counter expires.
//
// So the above example would draw 3 sprites in succession, then break for three attempts, then three again etc.
#include <iostream>
void OSprites::sprite_control()
{
    uint16_t d0 = trackloader.read_scenery_pos();

    // Populate next road segment
    if (d0 <= oroad.road_pos >> 16)
    {
        seg_pos = d0;                                                 // Position In Level Data [Word]
        seg_total_sprites = trackloader.read_total_sprites();         // Number of Sprites In Segment
        d0 = trackloader.read_sprite_pattern_index();                 // Block Of Sprites

        trackloader.scenery_offset += 4;                              // Advance to next scenery point
        
        uint32_t a0 = trackloader.read_scenerymap_table(d0);          // Get Address of Scenery Pattern
        seg_sprite_freq = trackloader.read16(trackloader.scenerymap_data, &a0);
        seg_spr_offset2 = trackloader.read16(trackloader.scenerymap_data, &a0);
        //uint32_t a0 = roms.rom0p->read32(outrun.adr.sprite_master_table + d0); // Set a0 to new address from master table of addresses
        //seg_sprite_freq = roms.rom0p->read16(&a0);                    // Set Sprite Frequency Value
        //seg_spr_offset2 = roms.rom0p->read16(&a0);                    // Set Reload value for sprite info offset
        seg_spr_addr = a0;                                            // Set ROM address for sprite info lookup (x, y, type)
                                                                      // NOTE: Sets to value of a0 itself, not memory location
        seg_spr_offset1 = 0;                                          // And Clear the offset into the above table
    }

    // Process segment
    if (seg_total_sprites == 0) return;
    if (seg_pos > oroad.road_pos >> 16) return;

    // ------------------------------------------------------------------------
    // Sprite 1
    // ------------------------------------------------------------------------
    // Rotate 16 bit value left. Stick top bit in low bit.
    uint16_t carry = seg_sprite_freq & 0x8000;
    seg_sprite_freq = ((seg_sprite_freq << 1) | ((seg_sprite_freq & 0x8000) >> 15)) & 0xFFFF;

    if (carry)
    {
        seg_total_sprites--;
        seg_spr_offset1 -= 8; //  Decrement rom address offset to point at next sprite [8 byte boundary]
        if (seg_spr_offset1 < 0)
            seg_spr_offset1 = seg_spr_offset2; // Reload sprite info offset value
        olevelobjs.setup_sprites(0x10400);
    }
 
    if (seg_total_sprites == 0)
    {
        seg_pos++;
        return;
    }

    // ------------------------------------------------------------------------
    // Sprite 2 - Second Sprite is slightly set back from the first.
    // ------------------------------------------------------------------------
    carry = seg_sprite_freq & 0x8000;
    seg_sprite_freq = ((seg_sprite_freq << 1) | ((seg_sprite_freq & 0x8000) >> 15)) & 0xFFFF;

    if (carry)
    {
        seg_total_sprites--;
        seg_spr_offset1 -= 8; //  Decrement rom address offset to point at next sprite [8 byte boundary]
        if (seg_spr_offset1 < 0)
            seg_spr_offset1 = seg_spr_offset2; // Reload sprite info offset value
        olevelobjs.setup_sprites(0x10000);
    }

     seg_pos++;
}

// Source: 0x76F4
void OSprites::clear_palette_data()
{
    spr_col_pal = 0;
    for (int16_t i = 0; i < 0x100; i++)
        pal_lookup[i] = 0;
}


// Copy Sprite Palette Data To Palette RAM On Vertical Interrupt
// 
// Source Address: 0x858E
// Input:          Source address in rom of data format
// Output:         None
//

void OSprites::copy_palette_data()
{
    // Return if no palette entries to copy
    if (pal_copy_count <= 0) return;

    for (int16_t i = 0; i < pal_copy_count; i++)
    {
        // Palette Data Source Offset (aligned to start of 32 byte boundry, * 5)
        uint16_t src_offset = pal_addresses[(i * 2) + 0] << 5;
        uint32_t src_addr = 2 + PAL_DATA + src_offset; // Source address in ROM

        uint16_t dst_offset = pal_addresses[(i * 2) + 1] << 5;
        uint32_t dst_addr = 2 + PAL_SPRITES + dst_offset;

        // Move 28 Bytes from ROM to palette RAM
        for (uint16_t j = 0; j < 7; j++)
        {
            video.write_pal32(&dst_addr, roms.rom0.read32(&src_addr));
        }
    }
    pal_copy_count = 0; // All entries copied
}

// Map Palettes from ROM to Palette RAM for a particular sprite.
// 
// Source Address: 0x75EA
// Input:          Sprite
// Output:         None
//
// Prepares RAM for copy_palette_data routine on vint

// Notes:
// 1. Checks lookup table to determine whether relevant palette info is copied to ram. 
//    Return if already cached
// 2. Otherwise set the mapping between ROM and the HW Palette to be used
// 3. pal_copy_count contains the number of entries we need to copy
// 4. pal_addresses contains the address mapping

void OSprites::map_palette(oentry* spr)
{
    uint8_t pal = pal_lookup[spr->pal_src];

    // -----------------------------------
    // Entry is cached. Use entry.
    // -----------------------------------
    if (pal != 0)
    {
        spr->pal_dst = pal;
        return;
    }
    // -----------------------------------
    // Entry is not cached. Need to cache.
    // -----------------------------------

    // Increment hw colour palette entry to use
    if (++spr_col_pal > 0x7F) return;
    
    spr->pal_dst = spr_col_pal; // Set next available hw sprite colour palette
    pal_lookup[spr->pal_src] = spr_col_pal;

    pal_addresses[pal_copy_count * 2] = spr->pal_src;
    pal_addresses[(pal_copy_count * 2) + 1] = spr_col_pal;

    pal_copy_count++;
}

// Setup Sprite Ordering Table & Shadows
// 
// Source Address: 0x77A8
// Input:          Sprite To Copy
// Output:         None
//
// Notes:
// 1/ Reads Sprite-to-Sprite priority of individual sprite
// 2/ Creates ordered sprite table starting at 0x64000
// 3/ The format of this table is detailed in the comments at 0x78B0
// 4/ Optionally adds shadow to sprite if requires
//
// The end result is a table of sprite entries at 0x64000

void OSprites::do_spr_order_shadows(oentry* input)
{
    // Use priority as lookup into table. Assume we're on boundaries of 0x10
    uint16_t priority = (input->priority & 0x1FF) << 4;
    uint8_t bytes_to_copy = sprite_order[priority];

    // Maximum number of bytes we want to copy is 0x10
    if (bytes_to_copy < 0xE)
    {
        bytes_to_copy++;
        sprite_order[priority] = bytes_to_copy;
        sprite_order[priority + bytes_to_copy + 1] = input->jump_index; // put at offset +2
        spr_cnt_main++;
    }

    // Code to handle shadows under sprites
    // test_shadow: 
    if (!(input->control & SHADOW)) return;

    input->dst_index = spr_cnt_shadow;
    spr_cnt_shadow++;                       // Increment total shadow count
    
    uint8_t pal_dst = input->pal_dst;       // Backup Sprite Colour Palette
    uint8_t shadow = input->shadow;         // and priority and shadow settings
    int16_t x = input->x;                   // and x position
    uint32_t addr = input->addr;            // and original sprite data address
    input->pal_dst = 0;                     // clear colour palette
    input->shadow = 7;                      // Set NEW priority & shadow settings
    
    input->x += (input->road_priority * shadow_offset) >> 9; // d0 = sprite z / distance into screen

    if (input->control & TRAFFIC_SPRITE)
    {
        input->addr = outrun.adr.sprite_shadow_small;
        input->x = x;
    }
    else
    {
        input->addr = roms.rom0p->read32(outrun.adr.shadow_frames + 0x3C);
    }

    do_sprite(input);           // Create Shadowed Version Of Sprite For Hardware
    
    input->pal_dst = pal_dst;   // Restore Sprite Colour Palette
    input->shadow = shadow;     // ...and other values
    input->x = x;
    input->addr = addr;
}

// Sprite Copying Routine
// 
// Source Address: 0x78B0
// Input:          None
// Output:         None
//

void OSprites::sprite_copy()
{
    if (spr_cnt_main == 0)
    {
        finalise_sprites();
        return;
    }

    if (spr_cnt_main + spr_cnt_shadow > 0x7F)
    {
        spr_cnt_main = spr_cnt_shadow = 0;
        finalise_sprites();
        return;
    }

    uint32_t spr_cnt_main_copy = spr_cnt_main;

    // look up in sprite_order
    int32_t src_addr = -0x10;

    // copy to sprite_entries
    uint32_t dst_index = 0;

    // Get next relevant entry (number of bytes to copy, followed by indexes of sprites)
    while (spr_cnt_main_copy != 0)
    {
        src_addr += 0x10;
        uint8_t bytes_to_copy = sprite_order[src_addr]; // warning: actually reads a word here normally, so this is wrong

        // Copy the required number of bytes
        if (bytes_to_copy != 0)
        {
            int32_t src_offset = src_addr + 2;

            do
            {
                // Sprite Index To Draw
                sprite_order2[dst_index++] = sprite_order[src_offset++];                
                if (--spr_cnt_main_copy == 0) break; // Loop continues until this is zero.
            }
            // Decrement number of bytes to copy
            while (--bytes_to_copy != 0);
        }

        // Clear number of bytes to copy
        sprite_order[src_addr] = 0; 
    }

    // cont2:
    uint16_t cnt_shadow_copy = spr_cnt_shadow;

    // next_sprite
    for (uint16_t i = 0; i < spr_cnt_main; i++)
    {
        uint16_t jump_index = sprite_order2[i];
        oentry *entry = &jump_table[jump_index];
        entry->dst_index = cnt_shadow_copy;
        cnt_shadow_copy++;
        do_sprite(entry);
    }

    finalise_sprites();
}

// Was originally labelled set_end_marker
// 
// Source Address: 0x7942
// Input:          None
// Output:         None
//

void OSprites::finalise_sprites()
{
    sprite_count = spr_cnt_main + spr_cnt_shadow;
    
    // Set end sprite marker
    sprite_entries[sprite_count].data[0] = 0xFFFF;
    sprite_entries[sprite_count].data[1] = 0xFFFF;

    // TODO: Code to wait for interrupt goes here
    blit_sprites();
    otraffic.traffic_logic();
    otraffic.traffic_sound();
    spr_cnt_main = spr_cnt_shadow = 0;

    // Ready to swap buffer and blit
    do_sprite_swap = true;
}

// Copy Sprite Data to Sprite RAM 
// 
// Source Address: 0x97E4
// Input:          None
// Output:         None
//

void OSprites::blit_sprites()
{
    uint32_t dst_addr = SPRITE_RAM;

    for (uint16_t i = 0; i <= sprite_count; i++)
    {
        uint16_t* data = sprite_entries[i].data;

        // Write twelve bytes
        video.write_sprite16(&dst_addr, data[0]);
        video.write_sprite16(&dst_addr, data[1]);
        video.write_sprite16(&dst_addr, data[2]);
        video.write_sprite16(&dst_addr, data[3]);
        video.write_sprite16(&dst_addr, data[4]);
        video.write_sprite16(&dst_addr, data[5]);
        video.write_sprite16(&dst_addr, data[6]);

        // Allign on correct boundary
        dst_addr += 2;
    }
}

// Convert Sprite From Internal Software Format To Hardware Format
// 
// Source Address: 0x94EC
// Input:          Sprite To Copy
// Output:         None
//
// 1. Copies Sprite Information From Jump Table Area To RAM
// 2. Stores In Similar Format To Sprite Hardware, but with 4 extra bytes of scratch data on end
// 3. Note: Mostly responsible for setting x,y,width,height,zoom,pitch,priorities etc.
//
// 0x11ED2: Table of Sprite Addresses for Hardware. Contains:
//
// 5 x 10 bytes. One block for each sprite size lookup. 
// The exact sprite is selected using the ozoom_lookup.hpp table.
//
// + 0 : [Byte] Unused
// + 1 : [Byte] Width Helper Lookup  [Offsets into 0x20000 (the width and height table)]
// + 2 : [Byte] Line Data Width
// + 3 : [Byte] Height Helper Lookup [Offsets into 0x20000 (the width and height table)]
// + 4 : [Byte] Line Data Height
// + 5 : [Byte] Sprite Pitch
// + 7 : [Byte] Sprite Bank
// + 8 : [Word] Offset Within Sprite Bank

void OSprites::do_sprite(oentry* input)
{
    input->control |= DRAW_SPRITE; // Display input sprite

    // Get Correct Output Entry
    osprite* output = &sprite_entries[input->dst_index];

    // Copy address sprite was copied from.
    // todo: pass pointer?
    output->scratch = input->jump_index;

    // Hide Sprite if zoom lookup not set
    if (input->zoom == 0)
    {
        hide_hwsprite(input, output);
        return;
    }

    // Sprite Width/Height Settings
    uint16_t width = 0;
    uint16_t height = 0;
    
    // Set real h/v zoom values
    uint32_t index = (input->zoom * 4);
    output->set_vzoom(ZOOM_LOOKUP[index]); // note we don't increment src_rom here
    output->set_hzoom(ZOOM_LOOKUP[index++]);

    // -------------------------------------------------------------------------
    // Set width & height values using lookup
    // -------------------------------------------------------------------------
    uint16_t lookup_mask = ZOOM_LOOKUP[index++]; // Width/Height lookup helper
    
    // This is the address of the frame required for the level of zoom we're using
    // There are 5 unique frames that are typically used for zoomed sprites.
    // which correspond to different screen sizes
    uint32_t src_offsets = input->addr + ZOOM_LOOKUP[index];

    uint16_t d0 = input->draw_props | (input->zoom << 8);
    uint16_t top_bit = d0 & 0x8000;
    d0 &= 0x7FFF; // Clear top bit
    
    if (top_bit == 0)
    {
        if (ZOOM_LOOKUP[index] != 0)
        {
            lookup_mask += 0x4000;
            d0 = lookup_mask;
        }

        d0 = (d0 & 0xFF00) + roms.rom0p->read8(src_offsets + 1);
        width = roms.rom0p->read8(WH_TABLE + d0);
        d0 = (d0 & 0xFF00) + roms.rom0p->read8(src_offsets + 3);
        height = roms.rom0p->read8(WH_TABLE + d0);
    }
    // loc_9560:
    else
    {
        d0 &= 0x7C00;
        uint16_t h = d0;

        d0 = (d0 & 0xFF00) + roms.rom0p->read8(src_offsets + 1);
        width = roms.rom0p->read8(WH_TABLE + d0);
        d0 &= 0xFF;
        width += d0;
        
        h |= roms.rom0p->read8(src_offsets + 3);
        height = roms.rom0p->read8(WH_TABLE + h);
        h &= 0xFF;
        height += h;

    }
    // loc 9582:
    input->width = width;
    
    // -------------------------------------------------------------------------
    // Set Sprite X & Y Values
    // -------------------------------------------------------------------------
    set_sprite_xy(input, output, width, height);
    
    // Here we need the entire value set by above routine, not just top 0x1FF mask!
    int16_t sprite_x1 = output->get_x();
    int16_t sprite_x2 = sprite_x1 + width;
    int16_t sprite_y1 = output->get_y();
    int16_t sprite_y2 = sprite_y1 + height;

    const uint16_t x1_bounds = 512 + config.s16_x_off;
    const uint16_t x2_bounds = 192 - config.s16_x_off;

    // Hide Sprite if off screen (note bug fix to solve shadow wrapping issue on original game)
    // I think this bug might be permanently fixed with the introduction of widescreen mode
    // as I had to change the storage size of the x-cordinate. 
    // Unsetting fix_bugs may no longer revert to the original behaviour.
    if (sprite_y2 < 256 || sprite_y1 > 479 ||
        sprite_x2 < x2_bounds || (config.engine.fix_bugs ? sprite_x1 >= x1_bounds : sprite_x1 > x1_bounds))
    {
        hide_hwsprite(input, output);
        return;
    }

    // -------------------------------------------------------------------------
    // Set Palette & Sprite Bank Information
    // -------------------------------------------------------------------------
    output->set_pal(input->pal_dst); // Set Sprite Colour Palette
    output->set_offset(roms.rom0p->read16(src_offsets + 8)); // Set Offset within selected sprite bank
    output->set_bank(roms.rom0p->read8(src_offsets + 7) << 1); // Set Sprite Bank Value

    // -------------------------------------------------------------------------
    // Set Sprite Height
    // -------------------------------------------------------------------------
    if (sprite_y1 < 256)
    {
        int16_t y_adj = -(sprite_y1 - 256);
        y_adj *= roms.rom0p->read16(src_offsets + 2); // Width of line data (Unsigned multiply)
        y_adj /= height; // Unsigned divide
        y_adj *= roms.rom0p->read16(src_offsets + 4); // Length of line data (Unsigned multiply)
        output->inc_offset(y_adj);
        output->data[0x0] = (output->data[0x0] & 0xFF00) | 0x100; // Mask on negative y index
        output->set_height((uint8_t) sprite_y2);
    }
    else
    {
        output->set_height((uint8_t) height);
    }
    
    // -------------------------------------------------------------------------
    // Set Sprite Height Taking Elevation Of Road Into Account For Clipping Purposes
    //
    // Word 0: Priority of section
    // Word 1: Height of section
    //
    // Source: 0x9602
    // -------------------------------------------------------------------------

    // Start of priority elevation data in road ram
    uint16_t road_y_index = oroad.road_p0 + 0x280;
    
    // Priority List Not Populated (Flat Elevation)
    if (oroad.road_y[road_y_index + 0] == 0 && oroad.road_y[road_y_index + 1] == 0)
    {
        // set_spr_height:
        if (sprite_y2 > 0x1DF)
            output->sub_height(sprite_y2 - 0x1DF);
    }
    // Priority List Populated (Elevated Section of road)
    else
    {
        // Count number of height_entries
        int16_t height_entry = 0;
        do
        {
            height_entry++;
            road_y_index += 2;
        }
        while (oroad.road_y[road_y_index + 0] != 0 && oroad.road_y[road_y_index + 1] != 0);

        height_entry--;

        do
        {
            road_y_index -= 2;
        }
        while (--height_entry > 0 && input->road_priority > oroad.road_y[road_y_index + 0]);

        // Sprite has higher priority, draw sprite
        if (input->road_priority > oroad.road_y[road_y_index])
        {
            // set_spr_height:
            if (sprite_y2 > 0x1DF)
                output->sub_height(sprite_y2 - 0x1DF);
        }
        // Road has higher priority, clip sprite
        else
        {
            // 9630:
            int16_t road_elevation = -oroad.road_y[road_y_index + 1] + 0x1DF;
            if (sprite_y1 > road_elevation)
            {
                hide_hwsprite(input, output);
                return;
            }
            else if (sprite_y2 <= road_elevation)
            {
                if (sprite_y2 > 0x1DF)
                    output->sub_height(sprite_y2 - 0x1DF);
            }
            else
            {
                output->sub_height(sprite_y2 - road_elevation);
            }
        }
    }

    // cont2:
    set_hrender(input, output, roms.rom0p->read16(src_offsets + 4), width);
    
    // -------------------------------------------------------------------------
    // Set Sprite Pitch & Priority
    // -------------------------------------------------------------------------
    output->set_pitch(roms.rom0p->read8(src_offsets + 5) << 1);
    output->set_priority(input->shadow << 4); // todo: where does this get set?
}

// Hide Input And Output Entry
void OSprites::hide_hwsprite(oentry* input, osprite* output)
{
    input->control &= ~DRAW_SPRITE; // Hide input sprite
    output->hide();
}

// Sets Sprite Render Point
// 
// Source Address: 0x967C
// Input:          Jump Table Entry, Output Sprite Entry, Width & Height
// Output:         Updated Sprite Output Entry
//

void OSprites::set_sprite_xy(oentry* input, osprite* output, uint16_t width, uint16_t height)
{
    uint8_t anchor = input->draw_props;

    // -------------------------------------------------------------------------
    // Set Y Render Point
    // -------------------------------------------------------------------------

    int16_t y = input->y;

    switch((anchor & 0xC) >> 2)
    {
        // Anchor Center
        case 0:
        case 3:
            height >>= 1;
            y -= height;
            output->set_y(y + 256);
            break;
            
        // Anchor Left
        case 1:
            output->set_y(y + 256);
            break;

        // Anchor Right
        case 2:
            y -= height;
            output->set_y(y + 256);
            break;
    }

    // -------------------------------------------------------------------------
    // Set X Render Point
    // -------------------------------------------------------------------------

    int16_t x = input->x;

    switch(anchor & 0x3)
    {
        // Anchor Center
        case 0:
        case 3:
            width >>= 1;
            x -= width;
            output->set_x(x + 352);
            break;
            
        // Anchor Left
        case 1:
            output->set_x(x + 352);
            break;

        // Anchor Right
        case 2:
            x -= width;
            output->set_x(x + 352);
            break;
    }
}

// Determines whether to render sprite left-to-right or right-to-left
// 
// Source Address: 0x96E4
// Input:          Jump Table Entry, Output Sprite Entry, Offset
// Output:         Updated Sprite Output Entry
//
void OSprites::set_hrender(oentry* input, osprite* output, uint16_t offset, uint16_t width)
{
    uint8_t props = 0x60;
    uint8_t anchor = input->draw_props;

    // Anchor Top Left: Set Backwards & Left To Right Render
    if (anchor & 1)
        props = 0x60;
    // Anchor Bottom Right: Set Forwards & Right To Left Render
    else if (anchor & 2 || input->x < 0)
        props = 0;

    if (input->control & HFLIP)
    {
        if (props == 0) props = 0x40; // If H-Flip & Right To Left Render: Read data backwards
        else props = 0x20; // If H-Flip && Left To Right: Render left to right & Read data forwards
    }

    // setup_bank:

    // if reading forwards, increment the offset
    if ((props & 0x40) == 0)
    {
        output->inc_offset(offset - 1);
    }

    // if right to left, increment the x position
    if ((props & 0x20) == 0)
    {
        output->inc_x(width);
    }

    props |= 0x80; // Set render top to bottom
    output->set_render(props);
}

// Helper function to vary the move distance, based on the current frame-rate.
void OSprites::move_sprite(oentry* sprite, uint8_t shift)
{
    uint32_t addr = SPRITE_ZOOM_LOOKUP + (((sprite->z >> 16) << 2) | sprite_scroll_speed);
    uint32_t value = roms.rom0.read32(addr) >> shift;

    if (config.tick_fps == 60)
        value >>= 1;
    else if (config.tick_fps == 120)
        value >>= 2;

    sprite->z += value;
}