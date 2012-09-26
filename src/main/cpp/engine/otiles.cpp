#include "engine/otiles.hpp"

OTiles otiles;

OTiles::OTiles(void)
{
}

OTiles::~OTiles(void)
{
}


// Write Tilemap Values To Hardware On Vertical Interrupt
// 
// Source Address: 0xD790
// Input:          None
// Output:         None

void OTiles::write_tilemap_hw(void)
{
    video.write_text16(HW_FG_HSCROLL, fg_h_scroll & 0x1FF);
    video.write_text16(HW_BG_HSCROLL, bg_h_scroll & 0x1FF);
    video.write_text16(HW_FG_VSCROLL, fg_v_scroll & 0x1FF);
    video.write_text16(HW_BG_VSCROLL, bg_v_scroll & 0x1FF);
    video.write_text16(HW_FG_PSEL, fg_psel);
    video.write_text16(HW_BG_PSEL, bg_psel);
}

// Draw Copyright Text to text ram
// 
// Source Address: 0x85EA
// Input:          None
// Output:         None

void OTiles::setup_palette_default(void)
{
    uint32_t src_addr = 0x16ED8;
    uint32_t pal_addr = 0x120000;

    // Write longs of palette data. Read from ROM.
    for (int i = 0; i <= 0x1F; i++)
    {
        video.write_pal32(&pal_addr, roms.rom0.read32(&src_addr));
    }
}

// Setup default palette for tilemaps for stages 1,3,5
// 
// Source Address: 0x8602
// Input:          None
// Output:         None

void OTiles::setup_palette_tilemap(void)
{
    uint32_t src_addr = 0x16FD8;
    uint32_t pal_addr = 0x120080;
    
    for (int i = 0; i <= 0x77; i++)
    {    
        uint16_t offset = roms.rom0.read8(&src_addr) << 4;
        uint32_t tile_data_addr = 0x17050 + offset;
        
        // Write 4 x longs of palette data. Read from ROM.
        video.write_pal32(&pal_addr, roms.rom0.read32(&tile_data_addr));
        video.write_pal32(&pal_addr, roms.rom0.read32(&tile_data_addr));
        video.write_pal32(&pal_addr, roms.rom0.read32(&tile_data_addr));
        video.write_pal32(&pal_addr, roms.rom0.read32(&tile_data_addr));
    }
}

// Reset Tiles, Palette And Road Split Data
//
// Source: 0xD7FC

void OTiles::reset_tiles_pal()
{
    tilemap_ctrl = TILEMAP_CLEAR;
    oinitengine.end_stage_props &= ~1; // Denote road not splitting
    opalette.pal_manip_ctrl = 0;
}

// Initialize, Scroll and Update Both FG & BG Tilemaps.
//
//
// Source Address: 0xD812
//
// Notes:
//
// Each level contains at FG and BG tilemap layer.
// Each tilemap comprises a number of name tables.
//
// There can be two FG and two BG layers loaded at once.
// This is because the previous and upcoming tilemaps are scrolled between on level switch.

void OTiles::update_tilemaps()
{
    if (outrun.service_mode) return;

    switch (tilemap_ctrl & 3)
    {
        // Clear Tile Table 1 & Init Default Tilemap (Stage 1)
        case TILEMAP_CLEAR:
            clear_tile_info();
            break;

        // Scroll Tilemap
        case TILEMAP_SCROLL:
            scroll_tilemaps();
            break;

        // Init Next Tilemap (on level switch)
        case TILEMAP_INIT:
            init_next_tilemap();
            break;

        // New Tilemap Initialized - Scroll both tilemaps during tilesplit
        case TILEMAP_SPLIT:
            split_tilemaps();
            break;
    }
}

// Clear various areas related to TILE RAM and init default values for start of level
// Source: 0xD848
void OTiles::clear_tile_info()
{
    // 1. Clear portion of RAM containing tilemap info (60F00 - 60F1F)
    fg_h_scroll = 
    bg_h_scroll = 
    fg_v_scroll = 
    bg_v_scroll =   // +4 words
    fg_psel = 
    bg_psel = 
    tilemap_v_scr = 
    tilemap_h_scr = // +5 words
    fg_v_tiles = 
    bg_v_tiles = 
    tilemap_v_off = // +3 words
    fg_addr = 
    bg_addr = 0;    // +4 words

    // 2. Clear portion of TEXT RAM containing tilemap info (110E80 - 110FFF)
    uint32_t dst_addr = HW_FG_PSEL;
    for (uint8_t i = 0; i <= 0x5F; i++)
        video.write_text32(&dst_addr, 0);

    // 3. Clear all of TILE RAM 100000 - 10FFFF
    video.clear_tile_ram();

    // 4. Setup new values
    fg_psel = roms.rom0.read16(TILES_PAGE_FG1);
    bg_psel = roms.rom0.read16(TILES_PAGE_BG1);
    video.write_text16(HW_FG_PSEL, fg_psel);    // Also write values to hardware
    video.write_text16(HW_BG_PSEL, bg_psel);

    init_default_tilemap();
}

// Initalize Default Tilemap (Stage 1)
//
// +0 [byte] - FG Tilemap Height
// +1 [byte] - BG Tilemap Height
// +2 [long] - FG Tilemap Address
// +6 [long] - BG Tilemap Address
// +A [word] - V-Scroll Offset
//
// Source: 0xD8B2
void OTiles::init_default_tilemap()
{
    fg_v_tiles    = roms.rom0.read8(TILES_TABLE);       // Write Default FG Tilemap Height
    bg_v_tiles    = roms.rom0.read8(TILES_TABLE + 1);   // Write Default BG Tilemap Height
    fg_addr       = roms.rom0.read32(TILES_TABLE + 2);  // Write Default FG Tilemap Address
    bg_addr       = roms.rom0.read32(TILES_TABLE + 6);  // Write Default BG Tilemap Address
    tilemap_v_off = roms.rom0.read16(TILES_TABLE + 0xA);
    int16_t v_off = 0x68 - tilemap_v_off;
    oroad.horizon_y_bak = oroad.horizon_y2;

    fg_v_scroll   = v_off;
    bg_v_scroll   = v_off;
    video.write_text16(HW_FG_VSCROLL, v_off);            // Also write values to hardware
    video.write_text16(HW_BG_VSCROLL, v_off);
    copy_fg_tiles(0x100F80);                            // Copy Foreground tiles to Tile RAM
    copy_bg_tiles(0x108F80);                            // Copy Background tiles to Tile RAM
    tilemap_ctrl = TILEMAP_SCROLL;
}


// Copy Foreground Tiles
// 
// - Initalise the foreground tilemap
// - Uncompress the tilemap from ROM and place into Tile RAM
// - The FG tilemap is defined by a 128x64 virtual name table, which is itself composed of four smaller 64x32 name tables.
//
// Source: 0xDCF2

void OTiles::copy_fg_tiles(uint32_t dst_addr)
{
    uint32_t src_addr = fg_addr;
    uint16_t offset = 0;          // Offset into Tile RAM (e.g. the name table to use)

    // Each tiled background is composed of 4 smaller 64x32 name tables. This counter iterates through them.
    for (uint8_t i = 0; i < 4; i++)
    {
        // next_name_table:
        uint32_t tileram_addr = dst_addr + offset;
        int16_t y = fg_v_tiles - 1;

        // next_tile_y:
        do
        {
            int16_t x = 0x3F;       // TILERAM is 0x40 Columns Wide x 8 pixels = 512
            // next_tilex:
            do
            {
                uint32_t data = roms.rom0.read16(&src_addr);

                // Compression
                if (data == 0)
                {
                    uint16_t value = roms.rom0.read16(&src_addr); // tile index to copy
                    uint16_t count = roms.rom0.read16(&src_addr); // number of times to copy value
                
                    // copy_compressed:
                    for (uint16_t i = 0; i <= count; i++)
                    {
                        video.write_tile16(&tileram_addr, value);
                        if (--x < 0)
                            break; // Break out of do/while loop to compression_done
                    }
                }
                // No Compression
                else
                {
                    // copy_next_word:
                    video.write_tile16(&tileram_addr, data);
                    --x;
                }
                // cont:
            }
            while (x >= 0);
            // compression_done:

            // Previous row in tileram (256 pixels)
            tileram_addr -= 0x100;
        }
        while (--y >= 0);

        offset += 0x1000; // Bytes between each name table
    }
}

// Copy Background Tiles
// 
// Note, this is virtually the same as the foreground method,
// aside from only copying 3 nametables, instead of 4.
//
// Source: 0xDD46
void OTiles::copy_bg_tiles(uint32_t dst_addr)
{
    uint32_t src_addr = bg_addr;
    uint16_t offset = 0;          // Offset into Tile RAM (e.g. the name table to use)

    // Each tiled background is composed of 3 smaller 64x32 name tables. This counter iterates through them.
    for (uint8_t i = 0; i < 3; i++)
    {
        // next_name_table:
        uint32_t tileram_addr = dst_addr + offset;
        int16_t y = bg_v_tiles - 1;

        // next_tile_y:
        do
        {
            int16_t x = 0x3F;       // TILERAM is 0x40 Columns Wide x 8 pixels = 512
            // next_tilex:
            do
            {
                uint32_t data = roms.rom0.read16(&src_addr);

                // Compression
                if (data == 0)
                {
                    uint16_t value = roms.rom0.read16(&src_addr); // tile index to copy
                    uint16_t count = roms.rom0.read16(&src_addr); // number of times to copy value
                
                    // copy_compressed:
                    for (uint16_t i = 0; i <= count; i++)
                    {
                        video.write_tile16(&tileram_addr, value);
                        if (--x < 0)
                            break; // Break out of do/while loop to compression_done
                    }
                }
                // No Compression
                else
                {
                    // copy_next_word:
                    video.write_tile16(&tileram_addr, data);
                    --x;
                }
                // cont:
            }
            while (x >= 0);
            // compression_done:

            // Previous row in tileram (256 pixels)
            tileram_addr -= 0x100;
        }
        while (--y >= 0);

        offset += 0x1000; // Bytes between each name table
    }
}

// Source: D910
void OTiles::scroll_tilemaps()
{
    // Yes, OutRun contains a lot of crappy code. Return when car moving into start line
    if (outrun.game_state != GS_BEST1 && outrun.game_state != GS_ATTRACT && outrun.game_state != GS_INGAME)
    {
        if (outrun.game_state != GS_BONUS) return;
    }

    // Determine if we need to loop back to Stage 1
    uint8_t loop = oinitengine.end_stage_props & 0x8;
    oinitengine.end_stage_props &= ~0x8;

    if (loop)
    {
        loop_to_stage1();
        return;
    }

    // Determine if road splitting
    if (oinitengine.end_stage_props & 1)
    {
        opalette.setup_sky_change();
        tilemap_ctrl = TILEMAP_INIT;
        tilemap_setup = SETUP_TILES;
    }
    // update_tilemap:
    // This block is called when we do not need to init a new tilemap

    // Do we need to clear the old name tables?
    if (clear_name_tables)
        clear_old_name_table();

    h_scroll_tilemaps();        // Set H-Scroll values for Tilemaps

    // Denote road not splitting (used by UpdateFGPage and UpdateBGPage)    
    if (oinitengine.rd_split_state == oinitengine.SPLIT_NONE)
        page_split = false;

    update_fg_page();           // Update FG Pages, based on new H-Scroll
    update_bg_page();           // Update BG Pages, based on new H-Scroll
    v_scroll_tilemaps();        // Set V-Scroll values for Tilemaps

}

// Note this is called in attract mode, when we need to loop back to Stage 1, from the final stage.
// Source: 0xD982
void OTiles::loop_to_stage1()
{
    opalette.pal_manip_ctrl = 1;    // Enable palette fade routines to transition between levels
    init_default_tilemap();         // Initalize Default Tilemap (Stage 1)
    opalette.setup_sky_change();    // Setup data in RAM necessary for sky palette fade.
}

// Clear the name tables used by the previous stage's tilemap, which aren't needed anymore
// Source: 0xDC3E
void OTiles::clear_old_name_table()
{
    clear_name_tables = false; // Denote tilemaps have been cleared

    // Odd Stages
    if (ostats.cur_stage & 1)
    {
        // Clear FG Tiles 2 [4 pages, (each 64x32 page table)]
        for (uint32_t i = 0x104C00; i < 0x108C00; i += 2)
            video.write_tile16(i, 0);

        // Clear BG Tiles 2 [3 pages]
        for (uint32_t i = 0x10B700; i < 0x10E700; i += 2)
            video.write_tile16(i, 0);
    }
    // Even
    else
    {
        // Clear FG Tiles 1 [4 pages, (each 64x32 page table)]
        for (uint32_t i = 0x100C00; i < 0x104C00; i += 2)
            video.write_tile16(i, 0);

        // Clear BG Tiles 1 [3 pages]
        for (uint32_t i = 0x108700; i < 0x10B700; i += 2)
            video.write_tile16(i, 0);
    }
}

// H-Scroll Tilemap Code
//
// Scroll the tilemaps. Note these are high level routines that don't write to the hardware directly.
//
// The first routine scrolls during the road-split, using a lookup table of predefined values.
// The second routine scrolls during normal gameplay.
// 
// Source: 0xDAA8

void OTiles::h_scroll_tilemaps()
{
    // Road Splitting
    if (oinitengine.end_stage_props & 1)
    {
        // Road position is used as an offset into the table. (Note it's reset at beginning of road split)
        h_scroll_lookup = roms.rom0.read16(H_SCROLL_TABLE + ((oroad.road_pos >> 16) << 1));
        
        int32_t tilemap_h_target = h_scroll_lookup << 5;
        tilemap_h_target <<= 16;
        int32_t tilemap_x = tilemap_h_target - (tilemap_h_scr << 5);
        if (tilemap_x != 0) 
        {
            tilemap_x >>= 8;
            if (tilemap_x == 0) 
                tilemap_h_scr = (tilemap_h_scr & 0xFFFF) | (h_scroll_lookup << 16);
            else
                tilemap_h_scr += tilemap_x;
            //std::cout << std::hex << "1a. tilemap_h_scr: " << tilemap_h_scr << " tilemap_x: " << tilemap_x << " tilemap_h_scr: " << oroad.tilemap_h_target << std::endl;
        }
        else
        {
            // DB1E
            tilemap_h_scr += (tilemap_x >> 8);
            //std::cout << std::hex << "2a. tilemap_h_scr: " << tilemap_h_scr << " tilemap_x: " << tilemap_x << " tilemap_h_scr: " << oroad.tilemap_h_target << std::endl;
        }
    }
    // Road Not Splitting
    else
    {
        // scroll_tilemap:
        if (oinitengine.rd_split_state != OInitEngine::SPLIT_NONE && 
            oinitengine.rd_split_state <= 4) return;

        int32_t tilemap_h_target = (oroad.tilemap_h_target << 5) & 0xFFFF;
        tilemap_h_target <<= 16;
        int32_t tilemap_x = tilemap_h_target - (tilemap_h_scr << 5);
        if (tilemap_x != 0) 
        {
            tilemap_x >>= 8;
            if (tilemap_x == 0) 
                tilemap_h_scr = (tilemap_h_scr & 0xFFFF) | (oroad.tilemap_h_target << 16);
            else
                tilemap_h_scr += tilemap_x;
            //std::cout << std::hex << "1b. tilemap_h_scr: " << tilemap_h_scr << " tilemap_x: " << tilemap_x << " tilemap_h_scr: " << oroad.tilemap_h_target << std::endl;
        }   
        else
        {
            // DB1E
            tilemap_h_scr += (tilemap_x >> 8);
            //std::cout << std::hex << "2b. tilemap_h_scr: " << tilemap_h_scr << " tilemap_x: " << tilemap_x << " tilemap_h_scr: " << oroad.tilemap_h_target << std::endl;
        }   
    }
}

// V-Scroll Tilemap Code
//
// Scroll the tilemaps. 
//
// Inputs:
// a6 = 0x60F00
//
// Source: 0xDBB8
void OTiles::v_scroll_tilemaps()
{
    oroad.horizon_y_bak = (oroad.horizon_y_bak + oroad.horizon_y2) >> 1;
    int32_t d0 = (0x100 - oroad.horizon_y_bak - tilemap_v_off);
    tilemap_v_scr ^= d0;

    if (d0 < 0)
    {
        fg_psel = (fg_psel >> 8) | ((fg_psel & 0xFF) << 8); // Swap
        bg_psel = (bg_psel >> 8) | ((bg_psel & 0xFF) << 8); // Swap
    }
    tilemap_v_scr = d0;         // Write d0 to master V-Scroll
    fg_v_scroll = d0;           // Write d0 to FG V-Scroll (ready for HW write)
    bg_v_scroll = d0;           // Write d0 to BG V-Scroll (ready for HW write)
}

// Update FG Page Values
//
// - Inverts H-Scroll if road splitting
// - Converts master H-Scroll value into values ready to be written to HW (not written here though)
// - Lookup Correct Page Select Of Background, based on H-Scroll
//
// This method assumes the new H-Scroll value has been set.
//
// Source: DB26

void OTiles::update_fg_page()
{
    int16_t h = tilemap_h_scr >> 16;
    if (oinitengine.rd_split_state == oinitengine.SPLIT_NONE)
        h = -h;

    fg_h_scroll = h;
    
    // Choose Page 0 - 3
    int32_t rol7 = h << 7;
    h = ((rol7 >> 16) & 3) << 1;
    
    uint8_t cur_stage = ostats.cur_stage;
    
    if (page_split)
        cur_stage++;

    cur_stage &= 1;
    cur_stage *= 8;
    h += cur_stage;
    fg_psel = roms.rom0.read16(TILES_PAGE_FG1 + h);
}

void OTiles::update_bg_page()
{
    int16_t h = tilemap_h_scr >> 16;
    if (oinitengine.rd_split_state == oinitengine.SPLIT_NONE)
        h = -h;

    h &= 0x7FF;
    h = (h + (h << 1)) >> 2;
    
    bg_h_scroll = h;

    // Choose Page 0 - 3
    int32_t rol7 = h << 7;
    h = ((rol7 >> 16) & 3) << 1;

    uint8_t cur_stage = ostats.cur_stage;

    if (page_split)
        cur_stage++;

    cur_stage &= 1;
    cur_stage = ((cur_stage * 2) + cur_stage) << 1;
    h += cur_stage;
    bg_psel = roms.rom0.read16(TILES_PAGE_BG1 + h);
}

// Initalize Next Tilemap. On Level Switch.
// Source: 0xD994
void OTiles::init_next_tilemap()
{
    h_scroll_lookup = 0;
    clear_name_tables = false;
    page_split = false;
    opalette.pal_manip_ctrl = 1;            // Enable palette fade routines to transition between levels
    
    switch (tilemap_setup & 1)
    {
        // Setup New Tilemaps
        // Note that we copy to a location in tileram depending on the level here, so that the upcoming BG & FG
        // tilemap is loaded onto alternate name tables in tile ram.
        case SETUP_TILES:
            init_tilemap_props(oroad.stage_lookup_off + 8);
            copy_fg_tiles(ostats.cur_stage & 1 ? 0x100F80 : 0x104F80);
            copy_bg_tiles(ostats.cur_stage & 1 ? 0x108F80 : 0x10BF80);
            tilemap_setup = SETUP_PAL;
            break;

        // Setup New Palettes
        case SETUP_PAL:
            init_tilemap_palette(oroad.stage_lookup_off + 8);
            tilemap_ctrl = TILEMAP_SPLIT;
            break;
    }
}

// Initialize Tilemap properties for Stage (FG & BG)
//
// - Width & Height of Tilemap
// - ROM Address of Tiles
// - V-Scroll Offset
// Source: DC02
void OTiles::init_tilemap_props(uint16_t stage_id)
{
    uint8_t offset = (roms.rom0.read8(TILES_DEF_LOOKUP + stage_id) << 2) * 3;
    uint32_t addr = TILES_TABLE + offset;

    fg_v_tiles    = roms.rom0.read8(&addr);   // Write Default FG Tilemap Height
    bg_v_tiles    = roms.rom0.read8(&addr);   // Write Default BG Tilemap Height
    fg_addr       = roms.rom0.read32(&addr);  // Write Default FG Tilemap Address
    bg_addr       = roms.rom0.read32(&addr);  // Write Default BG Tilemap Address
    tilemap_v_off = roms.rom0.read16(&addr);  // Set Tilemap v-scroll offset
    
}

// Copy relevant palette to Palette RAM, for new FG and BG layers.
//
// Source: 0xDD94
void OTiles::init_tilemap_palette(uint16_t stage_id)
{
    // Map stage ID to level 0-14
    uint8_t level = roms.rom0.read8(MAP_STAGE_ID + stage_id);

    switch (level)
    {
        case 0:
            return;

        // Stage 2
        case 1:
            copy_to_palram(2, TILEMAP_PALS + 0xC0, 0x120780);
            copy_to_palram(0, TILEMAP_PALS + 0x100, 0x1205F0);
            break;

        case 2:
            copy_to_palram(0, TILEMAP_PALS + 0x60, 0x1205F0);
            copy_to_palram(1, TILEMAP_PALS + 0x20, 0x1205A0);
            break;

        // Stage 3
        case 3:
            return;

        case 4:
            copy_to_palram(3, TILEMAP_PALS + 0xE0, 0x120600);
            break;

        case 5:
            return;

        // Stage 4
        case 6:
            return;

        case 7:
            copy_to_palram(3, TILEMAP_PALS, 0x1203C0);
            copy_to_palram(7, TILEMAP_PALS + 0x10, 0x1200C0);
            break;

        case 8:
            return;

        case 9:
            copy_to_palram(3, TILEMAP_PALS + 0x120, 0x120600);
            copy_to_palram(1, TILEMAP_PALS + 0x130, 0x1206C0);
            break;

        // Stage 5:
        case 10:
            copy_to_palram(1, TILEMAP_PALS + 0xF0, 0x1202A0);
            copy_to_palram(2, TILEMAP_PALS + 0x40, 0x120780);
            break;

        case 11:
            copy_to_palram(1, TILEMAP_PALS + 0x80, 0x1202A0);
            break;

        case 12:
            copy_to_palram(1, TILEMAP_PALS + 0x110, 0x1206C0);
            copy_to_palram(1, TILEMAP_PALS + 0x30, 0x120780);
            break;

        case 13:
            copy_to_palram(3, TILEMAP_PALS + 0x50, 0x120600);
            copy_to_palram(7, TILEMAP_PALS + 0x90, 0x1200C0);
            break;

        case 14:
            copy_to_palram(1, TILEMAP_PALS + 0xD0, 0x1202A0);
            copy_to_palram(1, TILEMAP_PALS + 0xB0, 0x120720);
            copy_to_palram(0, TILEMAP_PALS + 0xB0, 0x1207B0);
            break;
    }
}

void OTiles::copy_to_palram(const uint8_t blocks, uint32_t src, uint32_t dst)
{
    for (uint8_t i = 0; i <= blocks; i++)
    {
        video.write_pal32(&dst, roms.rom0.read32(src));
        video.write_pal32(&dst, roms.rom0.read32(src + 0x4));
        video.write_pal32(&dst, roms.rom0.read32(src + 0x8));
        video.write_pal32(&dst, roms.rom0.read32(src + 0xc));
    }
}

// New Tilemap Initialized - Scroll both tilemaps during tilesplit
//
// Source: 0xDA18
void OTiles::split_tilemaps()
{
    // Roads Splitting
    if (oinitengine.rd_split_state < 6)
    {
        h_scroll_tilemaps();
        update_fg_page_split();
        update_bg_page_split();
        v_scroll_tilemaps();
    }
    // Roads Merging
    else
    {
        tilemap_ctrl = TILEMAP_SCROLL;
        page_split = true;
        oinitengine.end_stage_props &= ~BIT_0; // Denote Road Split Ending
        h_scroll_lookup = 0;
        clear_name_tables = true; // Erase old tile name tables
    }
}

// Setup Foreground tilemap, with relevant h-scroll and page information. Ready for forthcoming HW write.
//
// Source: 0xDA54
void OTiles::update_fg_page_split()
{
    fg_h_scroll = tilemap_h_scr >> 16;
    fg_psel = roms.rom0.read16(TILES_PAGE_FG2 + ((ostats.cur_stage & 1) ? 0x6 : 0xE));
}

// Setup Background tilemap, with relevant h-scroll and page information. Ready for forthcoming HW write.
//
// Source: 0xDA78
void OTiles::update_bg_page_split()
{
    bg_h_scroll = (((tilemap_h_scr >> 16) & 0xFFF) * 3) >> 2;
    bg_psel = roms.rom0.read16(TILES_PAGE_BG2 + ((ostats.cur_stage & 1) ? 0x4 : 0xA));
}

// Fill tilemap background with a solid color
//
// Source: 0xE188
void OTiles::fill_tilemap_color(uint16_t color)
{
    uint32_t pal_addr   = 0x1204C2;
    uint32_t dst        = 0x10F000;
    const uint16_t TILE = color == 0 ? 0x20 : 0x1310;  // Default tile value for background

    reset_scroll();

    video.write_pal16(&pal_addr, color);

    for (uint16_t i = 0; i <= 0x7FF; i++)
        video.write_tile16(&dst, TILE);
}

// Helper function to reset scroll
void OTiles::reset_scroll()
{
    tilemap_ctrl = TILEMAP_SCROLL; // Use Palette
    fg_h_scroll = 0;
    bg_h_scroll = 0;
    fg_v_scroll = 0;
    bg_v_scroll = 0;

    fg_psel = 0xFFFF;
    bg_psel = 0xFFFF;
}