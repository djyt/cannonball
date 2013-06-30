#include "stdint.hpp"
#include "trackloader.hpp"
#include "roms.hpp"

#include "engine/oaddresses.hpp"

#include <fstream>
#include <iostream>

TrackLoader trackloader;

TrackLoader::TrackLoader()
{
    track_data = NULL;
    mode       = MODE_ORIGINAL;
}

TrackLoader::~TrackLoader()
{
    if (track_data != NULL)
        delete[] track_data;
}

void TrackLoader::init(const int mode)
{
    this->mode = mode;
}

void TrackLoader::setup_track(const uint32_t road_seg_master, const uint32_t road_height_lookup)
{
    if (mode == MODE_ORIGINAL)
    {
        curve_offset = roms.rom0p->read32(0 + road_seg_master); // Type of curve and unknown
	    wh_offset    = roms.rom0p->read32(4 + road_seg_master); // Width/Height Lookup
	    //road_seg_addr1 = roms.rom0p->read32(8 + road_seg_master); // Sprite information

        curve_data     = &roms.rom0p->rom[curve_offset];
        wh_data        = &roms.rom0p->rom[wh_offset];
        heightmap_data = roms.rom1p->rom;
        
        curve_offset     = 0;
        wh_offset        = 0;
        heightmap_offset = road_height_lookup;
    }
    else if (mode == MODE_CUSTOM)
    {
        wh_offset    = 0;
        curve_offset = 0;
    }
}

void TrackLoader::setup_path(const uint32_t stage_index)
{
    if (mode == MODE_ORIGINAL)
    {
        const uint32_t path_offset = roms.rom1p->read32(ROAD_DATA_LOOKUP + stage_index);
        path_data  = &roms.rom1p->rom[path_offset];
    }
    else if (mode == MODE_CUSTOM)
    {
    }
}

void TrackLoader::set_split()
{
    path_data  = &roms.rom1p->rom[ROAD_DATA_SPLIT];
}

void TrackLoader::set_bonus()
{
    path_data  = &roms.rom1p->rom[ROAD_DATA_BONUS];
}

uint32_t TrackLoader::read_heightmap_table(uint16_t entry)
{
    return read32(heightmap_data, heightmap_offset + (entry * 4));
}

int TrackLoader::load_level(const char* filename)
{
    std::string path = "roms/";
    path += std::string(filename);

    // Open rom file
    std::ifstream src(path.c_str(), std::ios::in | std::ios::binary);
    if (!src)
    {
        std::cout << "cannot open level: " << filename << std::endl;
        return 1; // fail
    }

    int length = filesize(path.c_str());

    // Read file
    char* buffer = new char[length];
    src.read(buffer, length);

    track_data = (uint8_t*) buffer;

    // Clean Up
    src.close();

    // Setup Data Blocks
    uint32_t addr = 0;
    path_data      = &track_data[read32(track_data, &addr)];
    curve_data     = &track_data[read32(track_data, &addr)];
    wh_data        = &track_data[read32(track_data, &addr)];
    heightmap_offset = read32(track_data, &addr);
    heightmap_data = track_data;

    mode       = MODE_CUSTOM;

    return 0; // success
}

int TrackLoader::filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::in | std::ifstream::binary);
    in.seekg(0, std::ifstream::end);
    int size = (int) in.tellg();
    in.close();
    return size; 
}