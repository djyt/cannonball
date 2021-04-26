/***************************************************************************
    Load OutRun ROM Set.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <iostream>
#include <cstring>
#include "stdint.hpp"
#include "roms.hpp"

Roms roms;

Roms::Roms()
{
    jap_rom_status = -1;
    rom0p = NULL;
    rom1p = NULL;
}

Roms::~Roms(void)
{
}

// Tidier way to address pointer to member function
// Expanded example: (rom0.*(rom0.load))("epr-10380b.133", 0x00000, 0x10000, 0x1f6cadad, RomLoader::INTERLEAVE2);
#define LOAD(rom, args) (rom.*(rom.load)) args

bool Roms::load_revb_roms(bool fixed_rom)
{
    // If incremented, a rom has failed to load.
    int status = 0;

    // Load Master CPU ROMs
    rom0.init(0x40000);
    status += LOAD(rom0, ("epr-10380b.133", 0x00000, 0x10000, 0x1f6cadad, RomLoader::INTERLEAVE2, VERBOSE));
    status += LOAD(rom0, ("epr-10382b.118", 0x00001, 0x10000, 0xc4c3fa1a, RomLoader::INTERLEAVE2, VERBOSE));
    status += LOAD(rom0, ("epr-10381b.132", 0x20000, 0x10000, 0xbe8c412b, RomLoader::INTERLEAVE2, VERBOSE));
    status += LOAD(rom0, ("epr-10383b.117", 0x20001, 0x10000, 0x10a2014a, RomLoader::INTERLEAVE2, VERBOSE));

    // Load Slave CPU ROMs
    rom1.init(0x40000);
    status += LOAD(rom1, ("epr-10327a.76", 0x00000, 0x10000, 0xe28a5baf, RomLoader::INTERLEAVE2, VERBOSE));
    status += LOAD(rom1, ("epr-10329a.58", 0x00001, 0x10000, 0xda131c81, RomLoader::INTERLEAVE2, VERBOSE));
    status += LOAD(rom1, ("epr-10328a.75", 0x20000, 0x10000, 0xd5ec5e5d, RomLoader::INTERLEAVE2, VERBOSE));
    status += LOAD(rom1, ("epr-10330a.57", 0x20001, 0x10000, 0xba9ec82a, RomLoader::INTERLEAVE2, VERBOSE));

    // Load Non-Interleaved Tile ROMs
    tiles.init(0x30000);
    status += LOAD(tiles, ("opr-10268.99", 0x00000, 0x08000, 0x95344b04, RomLoader::NORMAL, VERBOSE));
    status += LOAD(tiles, ("opr-10232.102", 0x08000, 0x08000, 0x776ba1eb, RomLoader::NORMAL, VERBOSE));
    status += LOAD(tiles, ("opr-10267.100", 0x10000, 0x08000, 0xa85bb823, RomLoader::NORMAL, VERBOSE));
    status += LOAD(tiles, ("opr-10231.103", 0x18000, 0x08000, 0x8908bcbf, RomLoader::NORMAL, VERBOSE));
    status += LOAD(tiles, ("opr-10266.101", 0x20000, 0x08000, 0x9f6f1a74, RomLoader::NORMAL, VERBOSE));
    status += LOAD(tiles, ("opr-10230.104", 0x28000, 0x08000, 0x686f5e50, RomLoader::NORMAL, VERBOSE));

    // Load Non-Interleaved Road ROMs (2 identical roms, 1 for each road)
    road.init(0x10000);
    status += LOAD(road, ("opr-10185.11", 0x000000, 0x08000, 0x22794426, RomLoader::NORMAL, VERBOSE));
    status += LOAD(road, ("opr-10186.47", 0x008000, 0x08000, 0x22794426, RomLoader::NORMAL, VERBOSE));

    // Load Interleaved Sprite ROMs
    sprites.init(0x100000);
    status += LOAD(sprites, ("mpr-10371.9", 0x000000, 0x20000, 0x7cc86208, RomLoader::INTERLEAVE4, VERBOSE));
    status += LOAD(sprites, ("mpr-10373.10", 0x000001, 0x20000, 0xb0d26ac9, RomLoader::INTERLEAVE4, VERBOSE));
    status += LOAD(sprites, ("mpr-10375.11", 0x000002, 0x20000, 0x59b60bd7, RomLoader::INTERLEAVE4, VERBOSE));
    status += LOAD(sprites, ("mpr-10377.12", 0x000003, 0x20000, 0x17a1b04a, RomLoader::INTERLEAVE4, VERBOSE));
    status += LOAD(sprites, ("mpr-10372.13", 0x080000, 0x20000, 0xb557078c, RomLoader::INTERLEAVE4, VERBOSE));
    status += LOAD(sprites, ("mpr-10374.14", 0x080001, 0x20000, 0x8051e517, RomLoader::INTERLEAVE4, VERBOSE));
    status += LOAD(sprites, ("mpr-10376.15", 0x080002, 0x20000, 0xf3b8f318, RomLoader::INTERLEAVE4, VERBOSE));
    status += LOAD(sprites, ("mpr-10378.16", 0x080003, 0x20000, 0xa1062984, RomLoader::INTERLEAVE4, VERBOSE));

    // Load Z80 Sound ROM
    // Note: This is a deliberate decision to double the Z80 ROM Space to accomodate extra FM based music
    z80.init(0x10000);
    status += LOAD(z80, ("epr-10187.88", 0x0000, 0x08000, 0xa10abaa9, RomLoader::NORMAL, VERBOSE));

    // Load Sega PCM Chip Samples
    pcm.init(0x60000);
    status += LOAD(pcm, ("opr-10193.66", 0x00000, 0x08000, 0xbcd10dde, RomLoader::NORMAL, VERBOSE));
    status += LOAD(pcm, ("opr-10192.67", 0x10000, 0x08000, 0x770f1270, RomLoader::NORMAL, VERBOSE));
    status += LOAD(pcm, ("opr-10191.68", 0x20000, 0x08000, 0x20a284ab, RomLoader::NORMAL, VERBOSE));
    status += LOAD(pcm, ("opr-10190.69", 0x30000, 0x08000, 0x7cab70e2, RomLoader::NORMAL, VERBOSE));
    status += LOAD(pcm, ("opr-10189.70", 0x40000, 0x08000, 0x01366b54, RomLoader::NORMAL, VERBOSE));
    status += LOAD(pcm, ("opr-10188.71", 0x50000, 0x08000, 0xbad30ad9, RomLoader::NORMAL, VERBOSE));
    status += load_pcm_rom(fixed_rom);

    // If status has been incremented, a rom has failed to load.
    return status == 0;
}

bool Roms::load_japanese_roms()
{
    // Only attempt to initalize the arrays once.
    if (jap_rom_status == -1)
    {
        j_rom0.init(0x40000);
        j_rom1.init(0x40000);
    }

    // If incremented, a rom has failed to load.
    jap_rom_status = 0;

    // Load Master CPU ROMs     
    jap_rom_status += LOAD(j_rom0, ("epr-10380.133", 0x00000, 0x10000, 0xe339e87a, RomLoader::INTERLEAVE2, VERBOSE));
    jap_rom_status += LOAD(j_rom0, ("epr-10382.118", 0x00001, 0x10000, 0x65248dd5, RomLoader::INTERLEAVE2, VERBOSE));
    jap_rom_status += LOAD(j_rom0, ("epr-10381.132", 0x20000, 0x10000, 0xbe8c412b, RomLoader::INTERLEAVE2, VERBOSE));
    jap_rom_status += LOAD(j_rom0, ("epr-10383.117", 0x20001, 0x10000, 0xdcc586e7, RomLoader::INTERLEAVE2, VERBOSE));

    // Load Slave CPU ROMs        
    jap_rom_status += LOAD(j_rom1, ("epr-10327.76", 0x00000, 0x10000, 0xda99d855, RomLoader::INTERLEAVE2, VERBOSE));
    jap_rom_status += LOAD(j_rom1, ("epr-10329.58", 0x00001, 0x10000, 0xfe0fa5e2, RomLoader::INTERLEAVE2, VERBOSE));
    jap_rom_status += LOAD(j_rom1, ("epr-10328.75", 0x20000, 0x10000, 0x3c0e9a7f, RomLoader::INTERLEAVE2, VERBOSE));
    jap_rom_status += LOAD(j_rom1, ("epr-10330.57", 0x20001, 0x10000, 0x59786e99, RomLoader::INTERLEAVE2, VERBOSE));
    // If status has been incremented, a rom has failed to load.
    return jap_rom_status == 0;
}

int Roms::load_pcm_rom(bool fixed_rom)
{
    int status = 0;
    if (fixed_rom)
    {
        status = LOAD(pcm, ("opr-10188.71f", 0x50000, 0x08000, 0x37598616, RomLoader::NORMAL, false));
        if (status == 1)
            status = LOAD(pcm, ("opr-10188.71f", 0x50000, 0x08000, 0xC2DE09B2, RomLoader::NORMAL, VERBOSE));
    }
    else
    {
        status = LOAD(pcm, ("opr-10188.71", 0x50000, 0x08000, 0xbad30ad9, RomLoader::NORMAL, VERBOSE));
    }

    //return LOAD(pcm, (fixed_rom ? "opr-10188.71f" : "opr-10188.71", 0x50000, 0x08000, fixed_rom ? 0x37598616 : 0xbad30ad9, RomLoader::NORMAL)) == 0;
    return status;
}

bool Roms::load_ym_data(const char* filename)
{
    RomLoader data;
    if (data.load_binary(filename) == 0)
    {
        if (data.length < 0x8000)
        {
            memcpy(z80.rom + 0x8000, data.rom, data.length);
            data.unload();
            return true;
        }
        else
        {
            std::cout << "YM Data is too large: " << filename << std::endl;
        }
    }
    return false;
}