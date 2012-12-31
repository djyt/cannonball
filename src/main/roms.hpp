/***************************************************************************
    Load OutRun ROM Set.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "romloader.hpp"

class Roms
{
public:

    // Western ROMs
    RomLoader rom0;
    RomLoader rom1;
    RomLoader tiles;
    RomLoader sprites;
    RomLoader road;
    RomLoader z80;
    RomLoader pcm;

    // Japanese ROMs
    RomLoader j_rom0;
    RomLoader j_rom1;

    // Paged Roms (Swap between Jap and Western)
    RomLoader* rom0p;
    RomLoader* rom1p;

    Roms();
    ~Roms();
    bool init();
    bool load_japanese_roms();
};

extern Roms roms;