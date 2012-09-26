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

    //RomLoader road_y;

    Roms();
    ~Roms();
    void init();
};

extern Roms roms;

