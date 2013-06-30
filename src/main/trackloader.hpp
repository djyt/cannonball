#pragma once

#include "stdint.hpp"

class TrackLoader
{

public:
    int mode;

    const static int MODE_ORIGINAL = 0;
    const static int MODE_CUSTOM   = 1;

    //uint32_t path_offset;
    uint32_t wh_offset;
    uint32_t curve_offset;
    uint32_t heightmap_offset;

    // Pointers into above track_data
    uint8_t* path_data;
    uint8_t* curve_data;
    uint8_t* wh_data;
    uint8_t* heightmap_data;

    TrackLoader();
    ~TrackLoader();
    void init(const int);
    void setup_track(const uint32_t, const uint32_t);
    void setup_path(const uint32_t);
    void set_split();
    void set_bonus();
    int load_level(const char* filename);
    uint32_t read_heightmap_table(uint16_t entry);

    inline int16_t readPath(uint32_t addr)
    {
        return (path_data[addr] << 8) | path_data[addr+1];
    }

    inline int16_t readPath(uint32_t* addr)
    {
        int16_t value = (path_data[*addr] << 8) | (path_data[*addr+1]);
        *addr += 2;
        return value;
    }

    inline int16_t read_width_height(uint32_t* addr)
    {
        int16_t value = (wh_data[*addr + wh_offset] << 8) | (wh_data[*addr+1 + wh_offset]);
        *addr += 2;
        return value;
    }

    inline int16_t read_curve(uint32_t addr)
    {
        return (curve_data[addr + curve_offset] << 8) | curve_data[addr+1 + curve_offset];
    }

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
    uint8_t* track_data; // Custom track data
    
    int filesize(const char* filename);
};

extern TrackLoader trackloader;