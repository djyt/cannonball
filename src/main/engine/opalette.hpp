/***************************************************************************
    Palette Control

    Sky, Ground & Road Palettes. Including palette cycling code.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.hpp"

class OPalette
{
public:

    // Palette Manipulation Control
    //
    // 0 = Palette manipulation deactivated
    //
    // Bit 0 = Set to enable/disable palette manipulation
    // Bit 1 = Set when fade difference calculated, and fade in progress
    // Bit 2 = Set when memory areas have been setup for fade
    uint8_t pal_manip_ctrl;

    OPalette(void);
    ~OPalette(void);
    void init();
	void setup_sky_palette();	
	void setup_sky_change();
    void setup_sky_cycle();
    void cycle_sky_palette();
    void fade_palette();
    void setup_ground_color();
	void setup_road_centre();
	void setup_road_stripes();
	void setup_road_side();
	void setup_road_colour();

private:
    // Sky Palette Manipulation Data (0x20 longs per palette, 0x1F separate palettes, then times 2 as for current/next sky on level transition)
    uint32_t pal_manip[(0x20 * 0x1F) * 2];

    // Palette Manipulation Area.
    //
    // Used for fades between colours when switching stages.
    //
    // Format:
    //
    // +00 [word] Current Palette Entry
    // +02 [word] Repacked RGB bits (from 06, 08, 0A) in final format
    // +04 [word] Next Palette Entry
    // +06 [word] Current BLUE bits  [i.e current fade, this is constantly adjusted]
    // +08 [word] Current GREEN bits [i.e current fade, this is constantly adjusted]
    // +0A [word] Current RED bits   [i.e current fade, this is constantly adjusted]
    // +0C [word] BLUE Difference between palette entries
    // +0E [word] GREEN Difference between palette entries
    // +10 [word] RED Difference between palette entries
    //(9 words per entry, 0x18 entries)
    uint16_t pal_fade[9 * 0x18];

    // Has new sky palette initalized (for level transitions)

    // Bit 0 = Set to denote that new sky palette info has been setup/copied to RAM. Cleared when bit 1 set.
    // Bit 1 = Set to indicate sky palette data should be cycled
    uint8_t sky_palette_init;

    // For palette fade manipulations
    uint8_t cycle_counter;

    // Fade counter - how many steps to fade the palette by
    int16_t fade_counter;

    // Sky Counter: Used to cycle sky palette on level transition
    uint16_t sky_palette_index;

    // Fade byte to setup (0 - 0x3F)
    uint8_t sky_fade_offset;

    void setup_fade_data();
    void repack_rgb(const uint32_t);
    void write_fade_to_palram();
    void write_current_pal_to_ram();
    void write_next_pal_to_ram();
    void fade_sky_pal_entry(const uint16_t, const uint16_t, uint32_t);
};

extern OPalette opalette;