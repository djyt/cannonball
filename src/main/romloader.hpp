/***************************************************************************
    Binary File Loader. 
    
    Handles loading an individual binary file to memory.
    Supports reading bytes, words and longs from this area of memory.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

class RomLoader
{

public:
    enum {NORMAL = 1, INTERLEAVE2 = 2, INTERLEAVE4 = 4};

    uint8_t* rom;

    // Size of rom
    uint32_t length;

    // Successfully loaded
    bool loaded;

    RomLoader();
    ~RomLoader();
    void init(uint32_t);
    int load(const char* filename, const int offset, const int length, const int expected_crc, const uint8_t mode = NORMAL);
    int load_binary(const char* filename);
    void unload(void);

    // ----------------------------------------------------------------------------
    // Used by translated 68000 Code
    // ----------------------------------------------------------------------------

    inline uint32_t read32(uint32_t* addr)
    {    
        uint32_t data = (rom[*addr] << 24) | (rom[*addr+1] << 16) | (rom[*addr+2] << 8) | (rom[*addr+3]);
        *addr += 4;
        return data;
    }

    inline uint16_t read16(uint32_t* addr)
    {
        uint16_t data = (rom[*addr] << 8) | (rom[*addr+1]);
        *addr += 2;
        return data;
    }

    inline uint8_t read8(uint32_t* addr)
    {
        return rom[(*addr)++]; 
    }

    inline uint32_t read32(uint32_t addr)
    {    
        return (rom[addr] << 24) | (rom[addr+1] << 16) | (rom[addr+2] << 8) | rom[addr+3];
    }

    inline uint16_t read16(uint32_t addr)
    {
        return (rom[addr] << 8) | rom[addr+1];
    }

    inline uint8_t read8(uint32_t addr)
    {
        return rom[addr];
    }

    // ----------------------------------------------------------------------------
    // Used by translated Z80 Code
    // Note that the endian is reversed compared with the 68000 code.
    // ----------------------------------------------------------------------------

    inline uint16_t read16(uint16_t* addr)
    {
        uint16_t data = (rom[*addr+1] << 8) | (rom[*addr]);
        *addr += 2;
        return data;
    }

    inline uint8_t read8(uint16_t* addr)
    {
        return rom[(*addr)++]; 
    }

    inline uint16_t read16(uint16_t addr)
    {
        return (rom[addr+1] << 8) | rom[addr];
    }

    inline uint8_t read8(uint16_t addr)
    {
        return rom[addr];
    }

private:
    int filesize(const char* filename);
};