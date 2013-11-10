/***************************************************************************
    Palette Control

    Sky, Ground & Road Palettes. Including palette cycling code.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "../trackloader.hpp"

#include "engine/ohud.hpp"
#include "engine/oinputs.hpp"
#include "engine/opalette.hpp"

OPalette opalette;

OPalette::OPalette(void)
{
}

OPalette::~OPalette(void)
{
}

void OPalette::init()
{
    sky_palette_init = 0;
    sky_fade_offset = 0;
    cycle_counter = 0;
}

// ----------------------------------------------------------------------------
//                              SKY PALETTE CHANGES
// ----------------------------------------------------------------------------

// Setup sky palette. Can be a shaded effect of 1F entries.
// Source: 8CA4
void OPalette::setup_sky_palette()
{
    // Read Address of sky palette information from index
    uint32_t src = trackloader.read_pal_sky_table(trackloader.current_level->pal_sky);
    uint32_t dst = 0x120F00; // palette ram

    for (int16_t i = 0; i <= 0x1F; i++)
        video.write_pal32(&dst, trackloader.read32(trackloader.pal_sky_data, &src));
}

// Setup data in RAM necessary for sky palette fade.
// Note this function also contains the Yu Suzuki Easter Egg.
//
// 1/ Copy current Sky Palette to RAM
// 2/ Copy upcoming Sky Palette to RAM
//
// Source: 0x8D06
void OPalette::setup_sky_change()
{
    uint32_t pal_addr = 0x120F00;

    for (int16_t i = 0; i <= 0x1F; i++)
        pal_manip[i] = video.read_pal32(&pal_addr);

    uint32_t stage_offset = oroad.stage_lookup_off;

    if (!(oinitengine.end_stage_props & BIT_2))
        stage_offset += 8;

    oinitengine.end_stage_props &= ~BIT_2; // Denote setup_sky_change done

    // Address Of Sky Palette Entries for next stage
    uint32_t src = trackloader.read_pal_sky_table(trackloader.get_level(stage_offset)->pal_sky);

    for (int16_t i = 0; i <= 0x1F; i++)
        pal_manip[i + 0x3E0] = trackloader.read32(trackloader.pal_sky_data, &src);

    sky_palette_init |= BIT_0; // Denote new sky palette setup

    // Easter Egg Code
    if (input.is_pressed(Input::START))
        ohud.blit_text1(TEXT1_EASTER);
}

// Sets up the palette cycle for the sky.
// There are 0x1E * complete palettes to handle the cycle.
// Each complete palette has 0x1F Long Entries
//
// Each time SetupSkyCycle is called it ammends all 0x1E palettes.
// But only ammends a single entry in each palette.
//
// The function cycle_sky_palette() takes the 0x1E palettes, and displays them in succession.
//
// Source: 0xBA68
void OPalette::setup_sky_cycle()
{
    if (sky_fade_offset == 0)
    {
        uint8_t is_copy_done = sky_palette_init & BIT_0;
        sky_palette_init &= ~BIT_0;
        if (!is_copy_done) return;
    }

    if (++sky_fade_offset > 0x40)
    {
        // Set to indicate sky palette data should be cycled
        sky_palette_init |= BIT_1;
        sky_fade_offset = 0;
        return;
    }

    uint16_t offset = (sky_fade_offset - 1);
    uint32_t dst_addr = (sky_fade_offset - 1) + 0x40; // We pass the word offset here, even though original code is bytes, and using a long array

    uint32_t start_colour = pal_manip[offset >> 1];
    uint32_t end_colour   = pal_manip[(offset >> 1) + 0x3E0];

    if (offset & 1)
    {
        start_colour &= 0xFFFF;
        end_colour &= 0xFFFF;
    }
    else
    {
        start_colour >>= 16;
        end_colour >>= 16;
    }
    fade_sky_pal_entry(start_colour, end_colour, dst_addr);    
 }

 //D15 : Shade hi/lo
 //D14 : Blue bit 0
 //D13 : Green bit 0
 //D12 : Red bit 0
 //D11 : Blue bit 4
 //D10 : Blue bit 3
 // D9 : Blue bit 2
 // D8 : Blue bit 1
 // D7 : Green bit 4
 // D6 : Green bit 3
 // D5 : Green bit 2
 // D4 : Green bit 1
 // D3 : Red bit 4
 // D2 : Red bit 3
 // D1 : Red bit 2
 // D0 : Red bit 1
 //
 // Source: 0xBACA

void OPalette::fade_sky_pal_entry(const uint16_t start_colour, const uint16_t end_colour, uint32_t addr)
{
    // START COLOUR
    // Extract bits 1-4
    uint16_t r1 = (start_colour & 0xF);
    uint16_t g1 = ((start_colour >> 4) & 0xF);
    uint16_t b1 = ((start_colour >> 8) & 0xF);

    // Mask on bit 0
    r1 = (r1 << 1) | ((start_colour & 0x1000) >> 12);
    g1 = (g1 << 1) | ((start_colour & 0x2000) >> 13);
    b1 = (b1 << 1) | ((start_colour & 0x4000) >> 14);

    r1 <<= 6;
    g1 <<= 6;
    b1 <<= 6;
    
    // END COLOUR
    // Extract bits 1-4
    uint16_t r2 = (end_colour & 0xF);
    uint16_t g2 = ((end_colour >> 4) & 0xF);
    uint16_t b2 = ((end_colour >> 8) & 0xF);

    // Mask on bit 0
    r2 = (r2 << 1) | ((end_colour & 0x1000) >> 12);
    g2 = (g2 << 1) | ((end_colour & 0x2000) >> 13);
    b2 = (b2 << 1) | ((end_colour & 0x4000) >> 14);

    r2 <<= 6;
    g2 <<= 6;
    b2 <<= 6;

    // MANIPULATE COLOURS
    r2 = (r2 - r1) >> 5;
    g2 = (g2 - g1) >> 5;
    b2 = (b2 - b1) >> 5;

    for (int16_t i = 0; i < 0x1E; i++)
    {
        r1 += r2;
        g1 += g2;
        b1 += b2;

        uint16_t r_bak = r1;
        uint16_t g_bak = g1;
        uint16_t b_bak = b1;

        // new value!
        uint16_t rgb = (((r1 >> 6) & 1) << 12) | (((g1 >> 6) & 1) << 13) | (((b1 >> 6) & 1) << 14); // bit 0
        r1 = ((r1 >> 7) & 0xF);
        g1 = ((g1 >> 7) & 0xF) << 4;
        b1 = ((b1 >> 7) & 0xF) << 8;
        rgb |= r1 | g1 | b1;

        // Hack due to using longs
        if (addr & 1)
            pal_manip[addr >> 1] = (pal_manip[addr >> 1] & 0xFFFF0000) | rgb;
        else
            pal_manip[addr >> 1] = (pal_manip[addr >> 1] & 0xFFFF) | (rgb << 16);

        addr += 0x40; // another hack

        r1 = r_bak;
        g1 = g_bak;
        b1 = b_bak;
    }
}

// - Cycle sky palette colours on level transition.
// - Uses the 0x1E palettes generated by setup_sky_cycle() 
//   (making 0x20 palettes total with the start and end palettes)
//
// Source: 0x8E20
void OPalette::cycle_sky_palette()
{
    if (!(sky_palette_init & BIT_1)) return;
    
    uint8_t d0 = ++cycle_counter;
    uint8_t d1 = d0 - 1;
    if (!((d0 ^ d1) & 1)) return;

    uint16_t pal_index = ++sky_palette_index;

    // Cleanup and finish if we've iterated through all 0x20 palettes
    if (pal_index >= 0x20)
    {
        sky_palette_init &= ~BIT_1; // Denote sky palette cycle done
        sky_palette_index = 0;
        if (outrun.game_state == GS_BEST1) return;
        // Clear Easter Egg Code
        ohud.blit_text1(TEXT1_EASTER_CLEAR);
        return;
    }
    // Otherwise, use index to select relevant palette of sky colours (d0 << 7)
    // Note that this aligns the palette to memory addresses at blocks of 0x80 bytes. (or 4*1F Longs)
    pal_index <<= 5;

    uint32_t pal_addr = 0x120F00; // dst

    for (int16_t i = 0; i <= 0x1F; i++)
        video.write_pal32(&pal_addr, pal_manip[i + pal_index]);
}

// ----------------------------------------------------------------------------
//                             GROUND PALETTE CHANGES
// ----------------------------------------------------------------------------

// Fade palettes between colours at the end of the level.
// Controls road and ground colours
// Source: 0x91F8
void OPalette::fade_palette()
{
    if (!(pal_manip_ctrl & BIT_0)) return;

    if (outrun.game_state != GS_ATTRACT && outrun.game_state != GS_INGAME) return;

    if (!(pal_manip_ctrl & BIT_1))
    {
        setup_fade_data();
        return;
    }

    // do_next_fade:
    uint16_t offset = 0;
    for (uint16_t i = 0; i < 0x18; i++)
    {
        pal_fade[offset + 3] += pal_fade[offset + 6]; // Adjust blue fade entry
        pal_fade[offset + 4] += pal_fade[offset + 7]; // Adjust green fade entry
        pal_fade[offset + 5] += pal_fade[offset + 8]; // Adjust red fade entry
        repack_rgb(offset);
        offset += 9;
    }

    write_fade_to_palram();

    if (--fade_counter == 0)
        pal_manip_ctrl = 0;
}

// Setup pal_manipulation data using colours from current and next level
//
// Source: 0x9270
void OPalette::setup_fade_data()
{
    uint32_t addr = 0;

    if (!(pal_manip_ctrl & BIT_2))
    {
        write_current_pal_to_ram();
        write_next_pal_to_ram();
    }
    // do_extract:
    else
    {
        addr = 0xD8 / 2;
    }

    // Iterate over 12 palette blocks
    for (int16_t i = 0; i < 12; i++)
    {
        uint16_t rgb = pal_fade[addr];
        uint16_t rgb_next = pal_fade[addr + 2];

        // 1/ Extract BLUE bits for current entry
        uint16_t d0 = (rgb & 0xF00) << 3;
        uint16_t d1 = (rgb & 0x4000) >> 4;
        uint16_t d2 = d0 | d1;
        pal_fade[addr + 3] = d2;

        // 2/ Extract BLUE bits for next entry (i.e. the colour for the upcoming level) and calculate blue difference
        d0 = (rgb_next & 0xF00) << 3;
        d1 = (rgb_next & 0x4000) >> 4;
        d0 = ((d0 | d1) - d2) >> 7;
        pal_fade[addr + 6] = d0;

        // 3/ Extract current GREEN bits
        d0 = (rgb & 0xF0) << 7;
        d1 = (rgb & 0x2000) >> 3;
        d2 = d0 | d1;
        pal_fade[addr + 4] = d2;

        // 4/ Extract next GREEN bits
        d0 = (rgb_next & 0xF0) << 7;
        d1 = (rgb_next & 0x2000) >> 3;
        d0 = ((d0 | d1) - d2) >> 7;
        pal_fade[addr + 7] = d0;

        // 5/ Extract current RED bits
        d0 = ((rgb & 0xF) << 11); // should be same as ror 5
        d1 = (rgb & 0x1000) >> 2;
        d2 = d0 | d1;
        pal_fade[addr + 5] = d2;

        // 6/ Extract next RED bits
        d0 = (rgb_next & 0xF) << 11;
        d1 = (rgb_next & 0x1000) >> 2;
        d0 = ((d0 | d1) - d2) >> 7;
        pal_fade[addr + 8] = d0;

        addr += 9;
    }

    // Memory areas setup for fade
    if (pal_manip_ctrl & BIT_2)
    {
        pal_manip_ctrl = 3; // Turn all bits on
        fade_counter = 0x80;
    }
    else
    {
        pal_manip_ctrl |= BIT_2;
    }
}

// Copy current palette ram data from 0x120800 - 0x12085F to normal RAM
// Source: 0x9372
void OPalette::write_current_pal_to_ram()
{
    uint32_t src = 0x120800; // pal_road_col1
    uint32_t dst = 0;    

    // Copy all of the road colour 1 stuff to RAM (0x120800 - 0x120810)  
    for (int16_t i = 0; i <= 7; i++)
    {
        pal_fade[dst] = video.read_pal16(&src);
        dst += 9;
    }

    src = 0x120840; // pal_ground1

    // Copy all of the palette ground 1 stuff (0x20 bytes total)
    for (int16_t i = 0; i <= 0xF; i++)
    {
        pal_fade[dst] = video.read_pal16(&src);
        dst += 9;
    }
}

// Copy palette of NEXT level to RAM at 0x66000
// Source: 0x9434
void OPalette::write_next_pal_to_ram()
{
    uint32_t dst = 2;
    uint32_t stage_offset = oroad.stage_lookup_off;

    if (!(oinitengine.end_stage_props & BIT_1))
        stage_offset += 8;

    oinitengine.end_stage_props &= ~BIT_1; 

    // Lookup palette entry from the road seg table based on route chosen
    Level *next_level = trackloader.get_level(stage_offset);

    // road palette
    pal_fade[dst] = next_level->palr1.road >> 16;             dst += 9;
    // sky palette
    pal_fade[dst] = next_level->palr1.road & 0xFFFF;          dst += 9;
    // Road Stripe Palette Entries
    pal_fade[dst] = next_level->palr1.stripe >> 16;           dst += 9;
    pal_fade[dst] = next_level->palr1.stripe & 0xFFFF;        dst += 9;
    // Road Side Palette Entries
    pal_fade[dst] = next_level->palr1.side >> 16;             dst += 9;
    pal_fade[dst] = next_level->palr1.side & 0xFFFF;          dst += 9;
    // Road Stripe Centre Palette Entries
    pal_fade[dst] = next_level->palr1.stripe_centre >> 16;    dst += 9;
    pal_fade[dst] = next_level->palr1.stripe_centre & 0xFFFF; dst += 9;
    
    // Ground Palette Entries (Index to table below)
    uint32_t ground_pal_addr = trackloader.read_pal_gnd_table(next_level->pal_gnd);

    for (int16_t i = 0; i <= 15; i++)
    {
        pal_fade[dst] = trackloader.read16(trackloader.pal_gnd_data, &ground_pal_addr);
        dst += 9;
    }
}

// Repack the individual RGB values into final word format.
//
// e.g.
//
// D14 : Blue bit 0
// D13 : Green bit 0
// D12 : Red bit 0
// D11 : Blue bit 4
// D10 : Blue bit 3
//  D9 : Blue bit 2
//  D8 : Blue bit 1
//  D7 : Green bit 4
//  D6 : Green bit 3
//  D5 : Green bit 2
//  D4 : Green bit 1
//  D3 : Red bit 4
//  D2 : Red bit 3
//  D1 : Red bit 2
//  D0 : Red bit 1
//
// Inputs:
//
// a6 = Palette Fade Entry Block Address
//
// Source: 0x93A2

void OPalette::repack_rgb(const uint32_t addr)
{
    pal_fade[addr + 1] = 0;
    
    // Pack BLUE bits
    uint16_t d0 = (pal_fade[addr + 3] >> 3) & 0xF00;
    uint16_t d1 = (pal_fade[addr + 3] << 4) & 0x4000;
    pal_fade[addr + 1] |= (d0 | d1);
    // Pack GREEN bits
    d0 = (pal_fade[addr + 4] >> 7) & 0xF0;
    d1 = (pal_fade[addr + 4] << 3) & 0x2000;
    pal_fade[addr + 1] |= (d0 | d1);
    // Pack RED bits
    d0 = (pal_fade[addr + 5] >> 11) & 0xF;
    d1 = (pal_fade[addr + 5] << 2) & 0x1000;
    pal_fade[addr + 1] |= (d0 | d1);
}

// Output new fade values to palette ram.
// Source: 0x93F0
void OPalette::write_fade_to_palram()
{
    uint32_t src = 1;
    uint32_t road1_pal_addr = 0x120800; // Road 1 palette
    uint32_t road2_pal_addr = 0x120810; // Road 2 palette

    for (int16_t i = 0; i < 8; i++)
    {
        video.write_pal16(&road1_pal_addr, pal_fade[src]);
        video.write_pal16(&road2_pal_addr, pal_fade[src]);
        src += 9;
    }

    road1_pal_addr = 0x120840;
    road2_pal_addr = 0x120860;

    for (int16_t i = 0; i < 16; i++)
    {
        video.write_pal16(&road1_pal_addr, pal_fade[src]);
        video.write_pal16(&road2_pal_addr, pal_fade[src]);
        src += 9;
    }
}

// ----------------------------------------------------------------------------
//                             SETUP DEFAULT COLOURS
// ----------------------------------------------------------------------------

// Initalise Colour Of Road Sides
// Source: 8ED2
void OPalette::setup_ground_color()
{
    // Read Address of ground palette information
    uint32_t src = trackloader.read_pal_gnd_table(trackloader.current_level->pal_gnd);
    uint32_t dst_pal_ground1 = 0x120840; // palette ram: ground 1
    uint32_t dst_pal_ground2 = 0x120860; // palette ram: ground 2

    for (int16_t i = 0; i < 8; i++)
    {
        uint32_t data = trackloader.read32(trackloader.pal_gnd_data, &src);
        video.write_pal32(&dst_pal_ground1, data);
        video.write_pal32(&dst_pal_ground2, data);
    }
}

void OPalette::setup_road_centre()
{
    video.write_pal32(0x12080C, trackloader.current_level->palr1.stripe_centre);
    video.write_pal32(0x12081C, trackloader.current_level->palr2.stripe_centre);
}

void OPalette::setup_road_stripes()
{
    video.write_pal32(0x120804, trackloader.current_level->palr1.stripe);
    video.write_pal32(0x120814, trackloader.current_level->palr2.stripe);
}

void OPalette::setup_road_side()
{
    video.write_pal32(0x120808, trackloader.current_level->palr1.side);
    video.write_pal32(0x120818, trackloader.current_level->palr2.side);
}

void OPalette::setup_road_colour()
{
    video.write_pal32(0x120800, trackloader.current_level->palr1.road);
    video.write_pal32(0x120810, trackloader.current_level->palr2.road);
}
