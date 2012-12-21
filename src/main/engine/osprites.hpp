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

#pragma once

#include "outrun.hpp"

class OSprites
{
public:

	enum 
    {
		HFLIP = 0x1,			// Bit 0: Horizontally flip sprite
		WIDE_ROAD = 0x4,		// Bit 2: Set if road_width >= 0x118, 
        TRAFFIC_SPRITE = 0x8,   // Bit 3: Traffic Sprite - Set for traffic
        SHADOW = 0x10,          // Bit 4: Sprite has shadows
		DRAW_SPRITE = 0x20,	    // Bit 5: Toggle sprite visibility
        TRAFFIC_RHS = 0x40,	    // Bit 6: Traffic Sprites - Set to spawn on RHS
		ENABLE = 0x80,	        // Bit 7: Toggle sprite visibility
	};

    // Note, the original game has 0x4F entries.
    // But we can set it to a higher value, to fix the broken arches on Gateway.
    // this is a limitation in the original game.
    // We just leave the larger array in place when changing the settings. 
    
    // Total sprite entries in Jump Table (start at offset #3)
	const static uint8_t SPRITE_ENTRIES = 0x62;
    
    // This is initalized based on the config
    uint8_t no_sprites;

    const static uint8_t SPRITE_FERRARI = SPRITE_ENTRIES + 1;
    const static uint8_t SPRITE_PASS1   = SPRITE_ENTRIES + 2;   // Passengers
    const static uint8_t SPRITE_PASS2   = SPRITE_ENTRIES + 3;
    const static uint8_t SPRITE_SHADOW  = SPRITE_ENTRIES + 4;   // Ferrari Shadow
    const static uint8_t SPRITE_SMOKE1  = SPRITE_ENTRIES + 5;   // Ferrari Tyre Smoke/Effects
    const static uint8_t SPRITE_SMOKE2  = SPRITE_ENTRIES + 6;
    const static uint8_t SPRITE_TRAFF1  = SPRITE_ENTRIES + 7;   // 8 Traffic Entries
    const static uint8_t SPRITE_TRAFF2  = SPRITE_ENTRIES + 8;
    const static uint8_t SPRITE_TRAFF3  = SPRITE_ENTRIES + 9;
    const static uint8_t SPRITE_TRAFF4  = SPRITE_ENTRIES + 10;
    const static uint8_t SPRITE_TRAFF5  = SPRITE_ENTRIES + 11;
    const static uint8_t SPRITE_TRAFF6  = SPRITE_ENTRIES + 12;
    const static uint8_t SPRITE_TRAFF7  = SPRITE_ENTRIES + 13;
    const static uint8_t SPRITE_TRAFF8  = SPRITE_ENTRIES + 14;
    
    const static uint8_t SPRITE_CRASH          = SPRITE_ENTRIES + 15;
    const static uint8_t SPRITE_CRASH_SHADOW   = SPRITE_ENTRIES + 16;
    const static uint8_t SPRITE_CRASH_PASS1    = SPRITE_ENTRIES + 17;
    const static uint8_t SPRITE_CRASH_PASS1_S  = SPRITE_ENTRIES + 18;
    const static uint8_t SPRITE_CRASH_PASS2    = SPRITE_ENTRIES + 19;
    const static uint8_t SPRITE_CRASH_PASS2_S  = SPRITE_ENTRIES + 20;

    const static uint8_t SPRITE_FLAG  = SPRITE_ENTRIES + 21;    // Flag Man

	// Jump Table Sprite Entries
	oentry jump_table[SPRITE_ENTRIES + 24]; // added 24 here for ferrari code and other bits

	// Converted sprite entries in RAM for hardware.
	osprite sprite_entries[SPRITE_ENTRIES + 24];

	// -------------------------------------------------------------------------
	// Function Holders
	// -------------------------------------------------------------------------
    enum
    {
        TRAFFIC_INIT = 0x10,       // Initalize Traffic Object
        TRAFFIC_ENTRY = 0x11,      // First 0x80 Positions Of Road
        TRAFFIC_TICK = 0x12        // Tick Normally
    };

	// -------------------------------------------------------------------------
	// Jump Table 2 Entries For Sprite Control
	// -------------------------------------------------------------------------

	// +22 [Word] Road Position For Next Segment Of Sprites
	uint16_t seg_pos;

	// +24 [Byte] Number Of Sprites In Segment  
	uint8_t seg_total_sprites;

	// +26 [Word] Sprite Frequency Bitmask
	uint16_t seg_sprite_freq;

	// +28 [Word] Sprite Info Offset - Start Value. Loaded Into 2A. 
	int16_t seg_spr_offset2;

	// +2A [Word] Sprite Info Offset
	int16_t seg_spr_offset1;

	// +2C [Long] Sprite Info Base - Lookup for Sprite X World, Sprite Y World, Sprite Type Table Info [8 byte boundary blocks in ROM]
	uint32_t seg_spr_addr;

	// -------------------------------------------------------------------------

	// Denote whether to swap sprite ram
	bool do_sprite_swap;

    // Speed at which sprites should scroll. Depends on granular position difference.
	uint16_t sprite_scroll_speed;

    // Shadow multiplication value (signed). Offsets the shadow from the sprite.
    // Adjusted by the tilemap horizontal scroll, so shadows change depending on how much we've scrolled left and right
    int16_t shadow_offset;

	// Number of sprites to draw for sprite drawing routine (sum of spr_cnt_main and spr_cnt_shadow).
	uint16_t sprite_count;

	// Number of sprites to draw
	uint16_t spr_cnt_main;

	// Number of shadows to draw
	uint16_t spr_cnt_shadow;

	OSprites(void);
	~OSprites(void);
	void init();
    void disable_sprites();
    void tick();

    void clear_palette_data();
	void copy_palette_data();
	void map_palette(oentry* spr);
	void sprite_copy();
	void blit_sprites();

	void do_spr_order_shadows(oentry*);
	void do_sprite(oentry*);
	void set_sprite_xy(oentry*, osprite*, uint16_t, uint16_t);
	void set_hrender(oentry*, osprite*, uint16_t, uint16_t);

    void move_sprite(oentry*, uint8_t);

private:

	// Start of Sprite RAM
	static const uint32_t SPRITE_RAM = 0x130000;

	// Palette Ram: Sprite Entries Start Here
	static const uint32_t PAL_SPRITES = 0x121000;

	// Store the next available sprite colour palette (0 - 7F)
	uint8_t spr_col_pal;

	// Stores number of palette entries to copy from rom to palram
	int16_t pal_copy_count; 

	// Palette Addresses. Used in conjunction with palette lookup table.
	// Originally stored between 0x61602 - 0x617FF in RAM
	//
	// Format:
	// Word 1: ROM Source Offset
	// Word 2: Palette RAM Destination Offset
	//
	// Word 3: ROM Source Offset
	// Word 4: Palette RAM Destination Offset
	//
	//etc.
	uint16_t pal_addresses[0x100]; // todo: rename to pal_mapping

	// Palette Lookup Table
	uint8_t pal_lookup[0x100];

	// Converted sprite entries in RAM for hardware.
	uint8_t sprite_order[0x2000];
	uint8_t sprite_order2[0x2000];

    void sprite_control();
	void hide_hwsprite(oentry*, osprite*);
	void finalise_sprites();
};

extern OSprites osprites;
