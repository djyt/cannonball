/***************************************************************************
    Load OutRun ROM Set.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "stdint.hpp"
#include "roms.hpp"

Roms roms;

Roms::Roms()
{
}

Roms::~Roms(void)
{
}

bool Roms::init()
{
    // If incremented, a rom has failed to load.
    int status = 0;

    // Load Master CPU ROMs
    rom0.init(0x40000);
    status += rom0.load("epr-10380b.133", 0x00000, 0x10000, 0x1f6cadad, RomLoader::INTERLEAVE2);
    status += rom0.load("epr-10382b.118", 0x00001, 0x10000, 0xc4c3fa1a, RomLoader::INTERLEAVE2);
    status += rom0.load("epr-10381a.132", 0x20000, 0x10000, 0xbe8c412b, RomLoader::INTERLEAVE2);
    status += rom0.load("epr-10383b.117", 0x20001, 0x10000, 0x10a2014a, RomLoader::INTERLEAVE2);

    // Load Slave CPU ROMs
    rom1.init(0x40000);
    status += rom1.load("epr-10327a.76", 0x00000, 0x10000, 0xe28a5baf, RomLoader::INTERLEAVE2);
    status += rom1.load("epr-10329a.58", 0x00001, 0x10000, 0xda131c81, RomLoader::INTERLEAVE2);
    status += rom1.load("epr-10328a.75", 0x20000, 0x10000, 0xd5ec5e5d, RomLoader::INTERLEAVE2);
    status += rom1.load("epr-10330a.57", 0x20001, 0x10000, 0xba9ec82a, RomLoader::INTERLEAVE2);

    // Load Non-Interleaved Tile ROMs
    tiles.init(0x30000);
    status += tiles.load("opr-10268.99",  0x00000, 0x08000, 0x95344b04);
    status += tiles.load("opr-10232.102", 0x08000, 0x08000, 0x776ba1eb);
    status += tiles.load("opr-10267.100", 0x10000, 0x08000, 0xa85bb823);
    status += tiles.load("opr-10231.103", 0x18000, 0x08000, 0x8908bcbf);
    status += tiles.load("opr-10266.101", 0x20000, 0x08000, 0x9f6f1a74);
    status += tiles.load("opr-10230.104", 0x28000, 0x08000, 0x686f5e50);

    // Load Non-Interleaved Road ROMs (2 identical roms, 1 for each road)
    road.init(0x10000);
    status += road.load("opr-10185.11", 0x000000, 0x08000, 0x22794426);
    status += road.load("opr-10186.47", 0x008000, 0x08000, 0x22794426);

    // Load Interleaved Sprite ROMs
    sprites.init(0x100000);
    status += sprites.load("mpr-10371.9",  0x000000, 0x20000, 0x7cc86208, RomLoader::INTERLEAVE4);
    status += sprites.load("mpr-10373.10", 0x000001, 0x20000, 0xb0d26ac9, RomLoader::INTERLEAVE4);
    status += sprites.load("mpr-10375.11", 0x000002, 0x20000, 0x59b60bd7, RomLoader::INTERLEAVE4);
    status += sprites.load("mpr-10377.12", 0x000003, 0x20000, 0x17a1b04a, RomLoader::INTERLEAVE4);
    status += sprites.load("mpr-10372.13", 0x080000, 0x20000, 0xb557078c, RomLoader::INTERLEAVE4);
    status += sprites.load("mpr-10374.14", 0x080001, 0x20000, 0x8051e517, RomLoader::INTERLEAVE4);
    status += sprites.load("mpr-10376.15", 0x080002, 0x20000, 0xf3b8f318, RomLoader::INTERLEAVE4);
    status += sprites.load("mpr-10378.16", 0x080003, 0x20000, 0xa1062984, RomLoader::INTERLEAVE4);

    // Load Z80 Sound ROM
    z80.init(0x10000);
    status += z80.load("epr-10187.88", 0x0000, 0x10000, 0xa10abaa9);

    // Load Sega PCM Chip Samples
    pcm.init(0x60000);
    status += pcm.load("opr-10193.66", 0x00000, 0x08000, 0xbcd10dde);
    status += pcm.load("opr-10192.67", 0x10000, 0x08000, 0x770f1270);
    status += pcm.load("opr-10191.68", 0x20000, 0x08000, 0x20a284ab);
    status += pcm.load("opr-10190.69", 0x30000, 0x08000, 0x7cab70e2);
    status += pcm.load("opr-10189.70", 0x40000, 0x08000, 0x01366b54);
    status += pcm.load("opr-10188.71", 0x50000, 0x08000, 0xbad30ad9);

    // If status has been incremented, a rom has failed to load.
    return status == 0;
}



