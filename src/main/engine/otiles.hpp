/***************************************************************************
    Tilemap Handling Code. 

    Logic for the foreground and background tilemap layers.

    - Read and render tilemaps
    - H-Scroll & V-Scroll
    - Palette Initialization
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.hpp"

// Forward definition of video for cyclic dependency
class video;

class OTiles
{
public:       
    // + 0x21: Tilemap Control
	// 0 = Clear Tile Table 1 & Init Default Tilemap (Stage 1)
	// 1 = Scroll Tilemap
	// 2 = Init Tilemap
	// 3 = New Tilemap Initialized - Scroll both tilemaps during tilesplit
	uint8_t tilemap_ctrl;
	enum { TILEMAP_CLEAR, TILEMAP_SCROLL, TILEMAP_INIT, TILEMAP_SPLIT };

	OTiles();
	~OTiles();

    void init();
    void set_vertical_swap();
	void setup_palette_tilemap();
    void setup_palette_widescreen();
	void setup_palette_hud();
    void reset_tiles_pal();
    void update_tilemaps(int8_t);
    void init_tilemap_palette(uint16_t);
    void fill_tilemap_color(uint16_t);
	void write_tilemap_hw();
    void set_scroll(int16_t h_scroll = 0, int16_t v_scroll = 0);

private:	
    // Page to use for tilemap. Alternates between 0 and 1 dependent on stage number
    // to handle switch between tilemaps at stage end.
    int8_t page;

    // Enhancement: Used for continuous mode
    int16_t vswap_state;
    enum {VSWAP_OFF, VSWAP_SCROLL_OFF, VSWAP_SCROLL_ON};

    int16_t vswap_off;

    // -----------------------------------------------------------------------
    // TILEMAP VARIABLES 
    // -----------------------------------------------------------------------

    // Scroll values to write to foreground & background tilemaps
	int16_t fg_h_scroll;
	int16_t bg_h_scroll;
	int16_t fg_v_scroll;
	int16_t bg_v_scroll;

	uint16_t fg_psel;
	uint16_t bg_psel;

	// + 0xC Current master tilemap scroll values
	int16_t tilemap_v_scr;
	int32_t tilemap_h_scr;

	// BG & FG Tilemap Height in Tiles
	uint16_t fg_v_tiles;
	uint16_t bg_v_tiles;

	// + 0x16 Tilemap v-scroll offset. Generally static.
	int16_t tilemap_v_off;

	// FG & BG Tilemap ROM Address [long]
	uint32_t fg_addr;
	uint32_t bg_addr;
	
    // + 0x20: Toggle between loading palette and loading tiles
    uint8_t tilemap_setup;
    enum { SETUP_TILES, SETUP_PAL };

    // + 0x22: Clear Old Name Tables
    bool clear_name_tables;

    // + 0x23: Set when road is splitting (used by UpdateFGPage and UpdateBGPage)
    bool page_split;

    // + 0x24: H-Scroll Lookup Table
    uint16_t h_scroll_lookup;

    // -----------------------------------------------------------------------
    
    void clear_tile_info();
    void init_tilemap(int16_t stage_id = 0);
    void init_tilemap_props(uint16_t);
    void scroll_tilemaps();
    void init_next_tilemap();
    void copy_to_palram(const uint8_t, uint32_t, uint32_t);
    void split_tilemaps();

    void loop_to_stage1();
    void clear_old_name_table();
    void h_scroll_tilemaps();
    void v_scroll_tilemaps();
    void copy_fg_tiles(uint32_t);
    void copy_bg_tiles(uint32_t);
    void update_fg_page();
    void update_bg_page();
    void update_fg_page_split();
    void update_bg_page_split();
};

extern OTiles otiles;

