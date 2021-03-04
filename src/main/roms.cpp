/***************************************************************************
    Load OutRun ROM Set.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "stdint.hpp"
#include "roms.hpp"

// ------------------------------------------------------------------------------------------------
// OutRun CRC 32 ROM Data
// ------------------------------------------------------------------------------------------------

// Master CPU ROMs (Original Japanese Release) 
static const int EPR_10380_133 = 0xe339e87a;
static const int EPR_10382_118 = 0x65248dd5;
static const int EPR_10381_132 = 0xbe8c412b;
static const int EPR_10383_117 = 0xdcc586e7;

// Slave CPU ROMs (Original Japanese Release)
static const int EPR_10327_76 = 0xda99d855;
static const int EPR_10329_58 = 0xfe0fa5e2;
static const int EPR_10328_75 = 0x3c0e9a7f;
static const int EPR_10330_57 = 0x59786e99;

// Master CPU ROMs (Rev B)
static const int EPR_10381b_132 = 0xbe8c412b;
static const int EPR_10383b_117 = 0x10a2014a;
static const int EPR_10380b_133 = 0x1f6cadad;
static const int EPR_10382b_118 = 0xc4c3fa1a;

// Slave CPU ROMs (Rev B)
static const int EPR_10327a_76 = 0xe28a5baf;
static const int EPR_10329a_58 = 0xda131c81;
static const int EPR_10328a_75 = 0xd5ec5e5d;
static const int EPR_10330a_57 = 0xba9ec82a;

// Non-Interleaved Tile ROMs
static const int OPR_10268_99  = 0x95344b04;
static const int OPR_10232_102 = 0x776ba1eb;
static const int OPR_10267_100 = 0xa85bb823;
static const int OPR_10231_103 = 0x8908bcbf;
static const int OPR_10266_101 = 0x9f6f1a74;
static const int OPR_10230_104 = 0x686f5e50;

// Non-Interleaved Road ROMs (2 identical roms, 1 for each road)
static const int OPR_10185_11  = 0x22794426;
static const int OPR_10186_47  = 0x22794426;

// Interleaved Sprite ROMs
static const int MPR_10371_9  = 0x7cc86208;
static const int MPR_10373_10 = 0xb0d26ac9;
static const int MPR_10375_11 = 0x59b60bd7;
static const int MPR_10377_12 = 0x17a1b04a;
static const int MPR_10372_13 = 0xb557078c;
static const int MPR_10374_14 = 0x8051e517;
static const int MPR_10376_15 = 0xf3b8f318;
static const int MPR_10378_16 = 0xa1062984;

// Z80 Sound ROM
static const int EPR_10187_88 = 0xa10abaa9;

// Load Sega PCM Chip Samples
static const int OPR_10193_66  = 0xbcd10dde;
static const int OPR_10192_67  = 0x770f1270;
static const int OPR_10191_68  = 0x20a284ab;
static const int OPR_10190_69  = 0x7cab70e2;
static const int OPR_10189_70  = 0x01366b54;
static const int OPR_10188_71  = 0xbad30ad9;
static const int OPR_10188_71f = 0x37598616; // Fixed version of shipped ROM

// ------------------------------------------------------------------------------------------------

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

bool Roms::load_revb_roms(bool fixed_rom)
{
    // If incremented, a rom has failed to load.
    int status = 0;

    // Load Master CPU ROMs
    rom0.init(0x40000);
    status += rom0.loadCRC32("EPR-10380b.133", EPR_10380b_133, 0x00000, 0x10000, RomLoader::INTERLEAVE2);
    status += rom0.loadCRC32("EPR-10382b.118", EPR_10382b_118, 0x00001, 0x10000, RomLoader::INTERLEAVE2);
    status += rom0.loadCRC32("EPR-10381b.132", EPR_10381b_132, 0x20000, 0x10000, RomLoader::INTERLEAVE2);    
    status += rom0.loadCRC32("EPR-10383b.117", EPR_10383b_117, 0x20001, 0x10000, RomLoader::INTERLEAVE2);

    // Load Slave CPU ROMs
    rom1.init(0x40000);
    status += rom1.loadCRC32("EPR-10327a.76", EPR_10327a_76, 0x00000, 0x10000, RomLoader::INTERLEAVE2);
    status += rom1.loadCRC32("EPR-10329a.58", EPR_10329a_58, 0x00001, 0x10000, RomLoader::INTERLEAVE2);
    status += rom1.loadCRC32("EPR-10328a.75", EPR_10328a_75, 0x20000, 0x10000, RomLoader::INTERLEAVE2);
    status += rom1.loadCRC32("EPR-10330a.57", EPR_10330a_57, 0x20001, 0x10000, RomLoader::INTERLEAVE2);

    // Load Non-Interleaved Tile ROMs
    tiles.init(0x30000);
    status += tiles.loadCRC32("OPR-10268.99", OPR_10268_99,  0x00000, 0x08000);
    status += tiles.loadCRC32("OPR-10232.102", OPR_10232_102, 0x08000, 0x08000);
    status += tiles.loadCRC32("OPR-10267.100", OPR_10267_100, 0x10000, 0x08000);
    status += tiles.loadCRC32("OPR-10231.103", OPR_10231_103, 0x18000, 0x08000);
    status += tiles.loadCRC32("OPR-10266.101", OPR_10266_101, 0x20000, 0x08000);
    status += tiles.loadCRC32("OPR-10230.104", OPR_10230_104, 0x28000, 0x08000);

    // Load Non-Interleaved Road ROMs (2 identical roms, 1 for each road)
    road.init(0x10000);
    status += road.loadCRC32("OPR-10185.11", OPR_10185_11, 0x000000, 0x08000);
    status += road.loadCRC32("OPR-10186.47", OPR_10186_47, 0x008000, 0x08000);

    // Load Interleaved Sprite ROMs
    sprites.init(0x100000);
    status += sprites.loadCRC32("MPR-10371.9", MPR_10371_9,  0x000000, 0x20000, RomLoader::INTERLEAVE4);
    status += sprites.loadCRC32("MPR-10373.10", MPR_10373_10, 0x000001, 0x20000, RomLoader::INTERLEAVE4);
    status += sprites.loadCRC32("MPR-10375.11", MPR_10375_11, 0x000002, 0x20000, RomLoader::INTERLEAVE4);
    status += sprites.loadCRC32("MPR-10377.12", MPR_10377_12, 0x000003, 0x20000, RomLoader::INTERLEAVE4);
    status += sprites.loadCRC32("MPR-10372.13", MPR_10372_13, 0x080000, 0x20000, RomLoader::INTERLEAVE4);
    status += sprites.loadCRC32("MPR-10374.14", MPR_10374_14, 0x080001, 0x20000, RomLoader::INTERLEAVE4);
    status += sprites.loadCRC32("MPR-10376.15", MPR_10376_15, 0x080002, 0x20000, RomLoader::INTERLEAVE4);
    status += sprites.loadCRC32("MPR-10378.16", MPR_10378_16, 0x080003, 0x20000, RomLoader::INTERLEAVE4);

    // Load Z80 Sound ROM
    z80.init(0x08000);
    status += z80.loadCRC32("EPR-10187.88", EPR_10187_88, 0x0000, 0x08000);

    // Load Sega PCM Chip Samples
    pcm.init(0x60000);
    status += pcm.loadCRC32("OPR-10193.66", OPR_10193_66, 0x00000, 0x08000);
    status += pcm.loadCRC32("OPR-10192.67", OPR_10192_67, 0x10000, 0x08000);
    status += pcm.loadCRC32("OPR-10191.68", OPR_10191_68, 0x20000, 0x08000);
    status += pcm.loadCRC32("OPR-10190.69", OPR_10190_69, 0x30000, 0x08000);
    status += pcm.loadCRC32("OPR-10189.70", OPR_10189_70, 0x40000, 0x08000);
    status += pcm.loadCRC32(fixed_rom ? "OPR-10188.71f" : "OPR-10188.71", fixed_rom ? OPR_10188_71f : OPR_10188_71, 0x50000, 0x08000);

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
    jap_rom_status += j_rom0.loadCRC32("EPR-10380.133", EPR_10380_133, 0x00000, 0x10000, RomLoader::INTERLEAVE2);
    jap_rom_status += j_rom0.loadCRC32("EPR-10382.118", EPR_10382_118, 0x00001, 0x10000, RomLoader::INTERLEAVE2);
    jap_rom_status += j_rom0.loadCRC32("EPR-10381.132", EPR_10381_132, 0x20000, 0x10000, RomLoader::INTERLEAVE2);
    jap_rom_status += j_rom0.loadCRC32("EPR-10383.117", EPR_10383_117, 0x20001, 0x10000, RomLoader::INTERLEAVE2);

    // Load Slave CPU ROMs        
    jap_rom_status += j_rom1.loadCRC32("EPR-10327.76", EPR_10327_76, 0x00000, 0x10000, RomLoader::INTERLEAVE2);
    jap_rom_status += j_rom1.loadCRC32("EPR-10329.58", EPR_10329_58, 0x00001, 0x10000, RomLoader::INTERLEAVE2);
    jap_rom_status += j_rom1.loadCRC32("EPR-10328.75", EPR_10328_75, 0x20000, 0x10000, RomLoader::INTERLEAVE2);
    jap_rom_status += j_rom1.loadCRC32("EPR-10330.57", EPR_10330_57, 0x20001, 0x10000, RomLoader::INTERLEAVE2);
    // If status has been incremented, a rom has failed to load.
    return jap_rom_status == 0;
}

bool Roms::load_pcm_rom(bool fixed_rom)
{
    return pcm.loadCRC32(fixed_rom ? "OPR-10188.71f" : "OPR-10188.71", fixed_rom ? OPR_10188_71f : OPR_10188_71, 0x50000, 0x08000);
}