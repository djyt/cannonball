/***************************************************************************
    Track Loading Code.

    Abstracts the level format, so that the original ROMs as well as
    in conjunction with track data from the LayOut editor.

    - Handles levels (path, width, height, scenery)
    - Handles additional level sections (road split, end section)
    - Handles road/level related palettes
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <iostream>
#include "trackloader.hpp"
#include "roms.hpp"
#include "engine/outrun.hpp"
#include "engine/oaddresses.hpp"

// ------------------------------------------------------------------------------------------------
// Stage Mapping Data: This is the master table that controls the order of the stages.
//
// You can change the stage order by editing this table.
// Bear in mind that the double lanes are hard coded in Stage 1.
//
// For USA there are unused tracks:
// 0x3A = Unused Coconut Beach
// 0x25 = Original Gateway track from Japanese edition
// 0x19 = Devils Canyon Variant
// ------------------------------------------------------------------------------------------------

static uint8_t STAGE_MAPPING_USA[] = 
{ 
    0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Stage 1
    0x1E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Stage 2
    0x20, 0x2F, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00,  // Stage 3
    0x2D, 0x35, 0x33, 0x21, 0x00, 0x00, 0x00, 0x00,  // Stage 4
    0x32, 0x23, 0x38, 0x22, 0x26, 0x00, 0x00, 0x00,  // Stage 5
};

static uint8_t STAGE_MAPPING_JAP[] = 
{ 
    0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Stage 1
    0x20, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Stage 2
    0x1E, 0x2F, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00,  // Stage 3
    0x2D, 0x25, 0x33, 0x21, 0x00, 0x00, 0x00, 0x00,  // Stage 4
    0x32, 0x23, 0x38, 0x22, 0x26, 0x00, 0x00, 0x00,  // Stage 5
};

TrackLoader trackloader;

TrackLoader::TrackLoader()
{
    layout        = NULL;
    levels        = new Level[STAGES];
    levels_end    = new Level[5];
    level_split   = new Level();
    current_level = &levels[0];

    mode       = MODE_ORIGINAL;
}

TrackLoader::~TrackLoader()
{
    if (layout != NULL)
        delete layout;

    delete[] levels_end;
    delete[] levels;
    delete level_split;
}

void TrackLoader::init(bool jap)
{
    if (mode == MODE_ORIGINAL)
        init_original_tracks(jap);
    else
        init_layout_tracks(jap);
}

bool TrackLoader::set_layout_track(const char* filename)
{
    if (layout == NULL)
        delete layout;

    layout = new RomLoader();
    
    if (layout->load_binary(filename))
        return false;

    mode = MODE_LAYOUT;

    return true;
}

void TrackLoader::init_original_tracks(bool jap)
{
    stage_data = jap ? STAGE_MAPPING_JAP : STAGE_MAPPING_USA;

    display_start_line = true;

    // --------------------------------------------------------------------------------------------
    // Setup Shared Data
    // --------------------------------------------------------------------------------------------

    // Height Map Entries
    heightmap_offset  = outrun.adr.road_height_lookup;
    heightmap_data    = &roms.rom1p->rom[0];  

    // Scenery Map Entries
    scenerymap_offset = outrun.adr.sprite_master_table;
    scenerymap_data   = &roms.rom0p->rom[0]; 

    // Palette Entries
    pal_sky_offset    = PAL_SKY_TABLE;
    pal_sky_data      = &roms.rom0.rom[0];

    pal_gnd_offset    = PAL_GND_TABLE;
    pal_gnd_data      = &roms.rom0.rom[0];

    // --------------------------------------------------------------------------------------------
    // Iterate and setup 15 stages
    // --------------------------------------------------------------------------------------------

    static const uint32_t STAGE_ORDER[] = { 0, 
                                            0x8, 0x9, 
                                            0x10, 0x11, 0x12, 
                                            0x18, 0x19, 0x1A, 0x1B, 
                                            0x20, 0x21, 0x22, 0x23, 0x24};

    for (int i = 0; i < STAGES; i++)
    {
        const uint16_t STAGE_OFFSET = stage_data[STAGE_ORDER[i]] << 2;

        // CPU 0 Data
        const uint32_t STAGE_ADR = roms.rom0p->read32(outrun.adr.road_seg_table + STAGE_OFFSET);
        setup_level(&levels[i], roms.rom0p, STAGE_ADR);

        // CPU 1 Data
        const uint32_t PATH_ADR = roms.rom1p->read32(ROAD_DATA_LOOKUP + STAGE_OFFSET);
        levels[i].path = &roms.rom1p->rom[PATH_ADR];        
    }

    // --------------------------------------------------------------------------------------------
    // Setup End Sections & Split Stages
    // --------------------------------------------------------------------------------------------

    // Split stages don't contain palette information
    setup_section(level_split, roms.rom0p, outrun.adr.road_seg_split);
    level_split->path         = &roms.rom1p->rom[ROAD_DATA_SPLIT];

    for (int i = 0; i < 5; i++)
    {
        const uint32_t STAGE_ADR = roms.rom0p->read32(outrun.adr.road_seg_end + (i << 2));
        setup_section(&levels_end[i], roms.rom0p, STAGE_ADR);
        levels_end[i].path  = &roms.rom1p->rom[ROAD_DATA_BONUS];
    }
}

void TrackLoader::init_layout_tracks(bool jap)
{
    stage_data = STAGE_MAPPING_USA;

    // --------------------------------------------------------------------------------------------
    // Check Version is Correct
    // --------------------------------------------------------------------------------------------
    if (layout->read32(LayOut::HEADER) != LayOut::EXPECTED_VERSION)
    {
        std::cout << "Incompatible LayOut Version Detected. Try upgrading CannonBall to the latest version" << std::endl;
        init_original_tracks(jap);
        return;
    }

    display_start_line = layout->read8((uint32_t)LayOut::HEADER + (uint32_t)sizeof(uint32_t));

    // --------------------------------------------------------------------------------------------
    // Setup Shared Data
    // --------------------------------------------------------------------------------------------

    // Height Map Entries
    heightmap_offset  = layout->read32(LayOut::HEIGHT_MAPS);
    heightmap_data    = &layout->rom[0];  

    // Scenery Map Entries
    scenerymap_offset = layout->read32(LayOut::SPRITE_MAPS);
    scenerymap_data   = &layout->rom[0]; 

    // Palette Entries
    pal_sky_offset    = layout->read32(LayOut::PAL_SKY);
    pal_sky_data      = &layout->rom[0];

    pal_gnd_offset    = layout->read32(LayOut::PAL_GND);
    pal_gnd_data      = &layout->rom[0];

    // --------------------------------------------------------------------------------------------
    // Iterate and setup 15 stages
    // --------------------------------------------------------------------------------------------
    for (int i = 0; i < STAGES; i++)
    {
        // CPU 0 Data
        const uint32_t STAGE_ADR = layout->read32(LayOut::LEVELS + (i * sizeof(uint32_t)));
        setup_level(&levels[i], layout, STAGE_ADR);

        // CPU 1 Data
        const uint32_t PATH_ADR = layout->read32(LayOut::PATH);
        levels[i].path = &layout->rom[ PATH_ADR + ((ROAD_END_CPU1 * sizeof(uint32_t)) * i) ];
    }

    // --------------------------------------------------------------------------------------------
    // Setup End Sections & Split Stages
    // --------------------------------------------------------------------------------------------

    // Split stages don't contain palette information
    setup_section(level_split, layout, layout->read32(LayOut::SPLIT_LEVEL));
    level_split->path = &layout->rom[ layout->read32(LayOut::SPLIT_PATH) ];

    // End sections don't contain palette information. Shared path.
    uint8_t* end_path = &layout->rom[ layout->read32(LayOut::END_PATH) ];
    for (int i = 0; i < 5; i++)
    {
        const uint32_t STAGE_ADR = layout->read32(LayOut::END_LEVELS + (i * sizeof(uint32_t)));
        setup_section(&levels_end[i], layout, STAGE_ADR);
        levels_end[i].path = end_path;
    }
}

// Setup a normal level
void TrackLoader::setup_level(Level* l, RomLoader* data, const int STAGE_ADR)
{
    // Sky Palette
    uint32_t adr = data->read32(STAGE_ADR + 0);
    l->pal_sky   = data->read16(adr);

    // Load Road Pallete
    adr = data->read32(STAGE_ADR + 4);
    l->palr1.stripe_centre = data->read32(&adr);
    l->palr2.stripe_centre = data->read32(adr);

    adr = data->read32(STAGE_ADR + 8);
    l->palr1.stripe = data->read32(&adr);
    l->palr2.stripe = data->read32(adr);

    adr = data->read32(STAGE_ADR + 12);
    l->palr1.side = data->read32(&adr);
    l->palr2.side = data->read32(adr);

    adr = data->read32(STAGE_ADR + 16);
    l->palr1.road = data->read32(&adr);
    l->palr2.road = data->read32(adr);

    // Ground Palette
    adr = data->read32(STAGE_ADR + 20);
    l->pal_gnd = data->read16(adr);

    // Curve Data
    curve_offset = data->read32(STAGE_ADR + 24);
    l->curve = &data->rom[curve_offset];

    // Width / Height Lookup
    wh_offset = data->read32(STAGE_ADR + 28);
    l->width_height = &data->rom[wh_offset];

    // Sprite Information
    scenery_offset = data->read32(STAGE_ADR + 32);
    l->scenery = &data->rom[scenery_offset];
}

// Setup a special section of track (end section or level split)
// Special sections do not contain palette information
void TrackLoader::setup_section(Level* l, RomLoader* data, const int STAGE_ADR)
{
    // Curve Data
    curve_offset = data->read32(STAGE_ADR + 0);
    l->curve = &data->rom[curve_offset];

    // Width / Height Lookup
    wh_offset = data->read32(STAGE_ADR + 4);
    l->width_height = &data->rom[wh_offset];

    // Sprite Information
    scenery_offset = data->read32(STAGE_ADR + 8);
    l->scenery = &data->rom[scenery_offset];
}

// ------------------------------------------------------------------------------------------------
//                                 CPU 0: Track Data (Scenery, Width, Height)
// ------------------------------------------------------------------------------------------------

void TrackLoader::init_track(const uint32_t offset)
{
    curve_offset   = 0;
    wh_offset      = 0;
    scenery_offset = 0;
    current_level  = &levels[stage_offset_to_level(offset)];
}

int8_t TrackLoader::stage_offset_to_level(uint32_t id)
{
    static const uint8_t ID_OFFSET[] = {0, 1, 3, 6, 10};
    return (ID_OFFSET[id / 8]) + (id & 7);
}


void TrackLoader::init_track_split()
{
    curve_offset   = 0;
    wh_offset      = 0;
    scenery_offset = 0;
    current_level  = level_split;
}

void TrackLoader::init_track_bonus(const uint32_t id)
{
    curve_offset   = 0;
    wh_offset      = 0;
    scenery_offset = 0;
    current_level  = &levels_end[id];
}

// ------------------------------------------------------------------------------------------------
//                                    CPU 1: Road Path Functions
// ------------------------------------------------------------------------------------------------

void TrackLoader::init_path(const uint32_t offset)
{
    current_path = levels[stage_offset_to_level(offset)].path;
}

void TrackLoader::init_path_split()
{
    current_path = level_split->path;
}

void TrackLoader::init_path_end()
{
    current_path = levels_end[0].path; // Path is shared for end sections
}

// ------------------------------------------------------------------------------------------------
//                                        HELPER FUNCTIONS TO READ DATA
// ------------------------------------------------------------------------------------------------

int16_t TrackLoader::readPath(uint32_t addr)
{
    return (current_path[addr] << 8) | current_path[addr+1];
}

int16_t TrackLoader::readPath(uint32_t* addr)
{
    int16_t value = (current_path[*addr] << 8) | (current_path[*addr+1]);
    *addr += 2;
    return value;
}

int16_t TrackLoader::read_width_height(uint32_t* addr)
{
    int16_t value = (current_level->width_height[*addr + wh_offset] << 8) | (current_level->width_height[*addr+1 + wh_offset]);
    *addr += 2;
    return value;
}

int16_t TrackLoader::read_curve(uint32_t addr)
{
    return (current_level->curve[addr + curve_offset] << 8) | current_level->curve[addr+1 + curve_offset];
}

uint16_t TrackLoader::read_scenery_pos()
{
    return (current_level->scenery[scenery_offset] << 8) | current_level->scenery[scenery_offset + 1];
}

uint8_t TrackLoader::read_total_sprites()
{
    return current_level->scenery[scenery_offset + 2];
}

uint8_t TrackLoader::read_sprite_pattern_index()
{
    return current_level->scenery[scenery_offset + 3];
}

Level* TrackLoader::get_level(uint32_t id)
{
    return &levels[stage_offset_to_level(id)];
}

uint32_t TrackLoader::read_pal_sky_table(uint16_t entry)
{
    return read32(pal_sky_data, pal_sky_offset + (entry * 4));
}

uint32_t TrackLoader::read_pal_gnd_table(uint16_t entry)
{
    return read32(pal_gnd_data, pal_gnd_offset + (entry * 4));
}

uint32_t TrackLoader::read_heightmap_table(uint16_t entry)
{
    return read32(heightmap_data, heightmap_offset + (entry * 4));
}

uint32_t TrackLoader::read_scenerymap_table(uint16_t entry)
{
    return read32(scenerymap_data, scenerymap_offset + (entry * 4));
}