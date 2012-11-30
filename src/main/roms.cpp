#include "stdint.hpp"
#include "roms.hpp"

/***************************************************************************
    Load OutRun ROM Set.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

Roms roms;

Roms::Roms()
{
}

Roms::~Roms(void)
{
}

void Roms::init()
{
    // Load Master CPU ROMs
    rom0.init(0x40000);
    rom0.load("roms/epr10380b.133", 0x00000, 0x10000, RomLoader::INTERLEAVE2);
    rom0.load("roms/epr10382b.118", 0x00001, 0x10000, RomLoader::INTERLEAVE2);
    rom0.load("roms/epr10381a.132", 0x20000, 0x10000, RomLoader::INTERLEAVE2);
    rom0.load("roms/epr10383b.117", 0x20001, 0x10000, RomLoader::INTERLEAVE2);

    // Load Slave CPU ROMs
    rom1.init(0x40000);
    rom1.load("roms/epr10327a.76", 0x00000, 0x10000, RomLoader::INTERLEAVE2);
    rom1.load("roms/epr10329a.58", 0x00001, 0x10000, RomLoader::INTERLEAVE2);
    rom1.load("roms/epr10328a.75", 0x20000, 0x10000, RomLoader::INTERLEAVE2);
    rom1.load("roms/epr10330a.57", 0x20001, 0x10000, RomLoader::INTERLEAVE2);

    // Load Non-Interleaved Tile ROMs
    tiles.init(0x30000);
    tiles.load("roms/opr10268.99", 0x00000, 0x08000);
    tiles.load("roms/opr10232.102", 0x08000, 0x08000);
    tiles.load("roms/opr10267.100", 0x10000, 0x08000);
    tiles.load("roms/opr10231.103", 0x18000, 0x08000);
    tiles.load("roms/opr10266.101", 0x20000, 0x08000);
    tiles.load("roms/opr10230.104", 0x28000, 0x08000);

    // Load Non-Interleaved Road ROMs (2 identical roms, 1 for each road)
    road.init(0x10000);
    road.load("roms/opr10185.11", 0x000000, 0x08000);
    road.load("roms/opr10186.47", 0x008000, 0x08000);

    // Load Interleaved Sprite ROMs
    sprites.init(0x100000);
    sprites.load("roms/mpr10371.9",  0x000000, 0x20000, RomLoader::INTERLEAVE4);
    sprites.load("roms/mpr10373.10", 0x000001, 0x20000, RomLoader::INTERLEAVE4);
    sprites.load("roms/mpr10375.11", 0x000002, 0x20000, RomLoader::INTERLEAVE4);
    sprites.load("roms/mpr10377.12", 0x000003, 0x20000, RomLoader::INTERLEAVE4);
    sprites.load("roms/mpr10372.13", 0x080000, 0x20000, RomLoader::INTERLEAVE4);
    sprites.load("roms/mpr10374.14", 0x080001, 0x20000, RomLoader::INTERLEAVE4);
    sprites.load("roms/mpr10376.15", 0x080002, 0x20000, RomLoader::INTERLEAVE4);
    sprites.load("roms/mpr10378.16", 0x080003, 0x20000, RomLoader::INTERLEAVE4);

    // Load Z80 Sound ROM
    z80.init(0x10000);
    z80.load("roms/epr10187.88", 0x0000, 0x10000);

    // Load Sega PCM Chip Samples
    pcm.init(0x60000);
    pcm.load("roms/opr10193.66", 0x00000, 0x08000);
    pcm.load("roms/opr10192.67", 0x10000, 0x08000);
    pcm.load("roms/opr10191.68", 0x20000, 0x08000);
    pcm.load("roms/opr10190.69", 0x30000, 0x08000);
    pcm.load("roms/opr10189.70", 0x40000, 0x08000);
    pcm.load("roms/opr10188.71", 0x50000, 0x08000);

    //road_y.init(0x2000);
    //road_y.load("roms/road_y.bin", 0, 0x2000);
}



