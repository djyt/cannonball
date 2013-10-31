#pragma once

#include "globals.hpp"

struct RoadPalette
{
    uint32_t stripe_centre;   // Centre Stripe Colour
    uint32_t stripe;          // Stripe Colour
    uint32_t side;            // Side Colour
    uint32_t road;            // Main Road Colour
};

struct Level
{
    uint8_t* path;            // CPU 1 Path Data
    uint8_t* curve;           // Track Curve Information (Derived From Path)
    uint8_t* width_height;    // Track Width & Height Lookups
    uint8_t* scenery;         // Track Scenery Lookups

    uint16_t pal_sky;         // Index into Sky Palettes
    uint16_t pal_gnd;         // Index into Ground Palettes

    RoadPalette palr1;        // Road 1 Generator Palette
    RoadPalette palr2;        // Road 2 Generator Palette
};

// LayOut Header Format
/*struct LayOut
{
    static const uint32_t EXPECTED_VERSION = 1;

    static const uint32_t HEADER      = 0;
    static const uint32_t PATH        = HEADER      + sizeof(uint32_t) + sizeof(uint8_t);
    static const uint32_t LEVELS      = PATH        + sizeof(uint32_t);
    static const uint32_t PAL_SKY     = LEVELS      + (STAGES * sizeof(uint32_t));
    static const uint32_t PAL_GND     = PAL_SKY     + sizeof(uint32_t);
    static const uint32_t SPRITE_MAPS = PAL_GND     + sizeof(uint32_t);
    static const uint32_t HEIGHT_MAPS = SPRITE_MAPS + sizeof(uint32_t);
};*/

struct LayOut
{
    static const uint32_t EXPECTED_VERSION = 1;

    static const uint32_t HEADER      = 0;
    static const uint32_t PATH        = HEADER      + sizeof(uint32_t) + sizeof(uint8_t);
    static const uint32_t LEVELS      = PATH        + sizeof(uint32_t);
    static const uint32_t SPLIT_PATH  = LEVELS      + (STAGES * sizeof(uint32_t));
    static const uint32_t SPLIT_LEVEL = SPLIT_PATH  + sizeof(uint32_t);
    static const uint32_t PAL_SKY     = SPLIT_LEVEL + sizeof(uint32_t);
    static const uint32_t PAL_GND     = PAL_SKY     + sizeof(uint32_t);
    static const uint32_t SPRITE_MAPS = PAL_GND     + sizeof(uint32_t);
    static const uint32_t HEIGHT_MAPS = SPRITE_MAPS + sizeof(uint32_t);
};

class RomLoader;

class TrackLoader
{

public:
    // Reference to stage mapping/ordering table
    uint8_t* stage_data;

    Level* current_level;

    const static int MODE_ORIGINAL = 0;
    const static int MODE_LAYOUT   = 1;

    uint8_t display_start_line;

    uint32_t curve_offset;
    uint32_t wh_offset;
    uint32_t scenery_offset;

    // Shared Structures
    uint8_t* pal_sky_data;
    uint8_t* pal_gnd_data;
    uint8_t* heightmap_data;
    uint8_t* scenerymap_data;

    uint32_t pal_sky_offset;
    uint32_t pal_gnd_offset;
    uint32_t heightmap_offset;
    uint32_t scenerymap_offset;

    TrackLoader();
    ~TrackLoader();

    void init();
    bool set_layout_track(const char* filename);
    void init_original_tracks();
    void init_layout_tracks();
    void init_track(const uint32_t);
    void init_track_split();
    void init_track_bonus(const uint32_t);

    void init_path(const uint32_t);
    void init_path_split();
    void init_path_bonus();

    uint32_t read_pal_sky_table(uint16_t entry);
    uint32_t read_pal_gnd_table(uint16_t entry);    
    uint32_t read_heightmap_table(uint16_t entry);
    uint32_t read_scenerymap_table(uint16_t entry);

    int16_t readPath(uint32_t addr);
    int16_t readPath(uint32_t* addr);
    int16_t read_width_height(uint32_t* addr);
    int16_t read_curve(uint32_t addr);
    uint16_t read_scenery_pos();
    uint8_t read_total_sprites();
    uint8_t read_sprite_pattern_index();

    int8_t stage_offset_to_level(uint32_t);
    Level* get_level(uint32_t);

    inline int32_t read32(uint8_t* data, uint32_t* addr)
    {    
        int32_t value = (data[*addr] << 24) | (data[*addr+1] << 16) | (data[*addr+2] << 8) | (data[*addr+3]);
        *addr += 4;
        return value;
    }

    inline int16_t read16(uint8_t* data, uint32_t* addr)
    {
        int16_t value = (data[*addr] << 8) | (data[*addr+1]);
        *addr += 2;
        return value;
    }

    inline int8_t read8(uint8_t* data, uint32_t* addr)
    {
        return data[(*addr)++]; 
    }

    inline int32_t read32(uint8_t* data, uint32_t addr)
    {    
        return (data[addr] << 24) | (data[addr+1] << 16) | (data[addr+2] << 8) | data[addr+3];
    }

    inline int16_t read16(uint8_t* data, uint32_t addr)
    {
        return (data[addr] << 8) | data[addr+1];
    }

    inline int8_t read8(uint8_t* data, uint32_t addr)
    {
        return data[addr];
    }


private:
    RomLoader* layout;

    int mode;

    Level* levels;
    Level* level_split;
    Level* levels_bonus;

    uint8_t* track_data; // Custom track data

    uint8_t* current_path; // CPU 1 
    
    void setup_level(Level* l, RomLoader* data, const int STAGE_ADR);
    void setup_split(Level* l, RomLoader* data, const int STAGE_ADR);
};

extern TrackLoader trackloader;