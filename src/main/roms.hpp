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

    RomLoader rom0;
    RomLoader rom1;
    RomLoader tiles;
    RomLoader sprites;
    RomLoader road;
    RomLoader z80;
    RomLoader pcm;

    Roms();
    ~Roms();
    bool init();
};

extern Roms roms;