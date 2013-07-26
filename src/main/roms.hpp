/***************************************************************************
    Load OutRun ROM Set.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "romloader.hpp"
#include "unzip/unzip.h"

#include <string>

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

    //RomLoader* track_data;
    //RomLoader custom_track;
    
    Roms();
    ~Roms();
    bool open_zip(const char* filename);
    void close_zip();
    bool load_revb_roms();
    bool load_japanese_roms();

private:
    unzFile zip_file;
    int jap_rom_status;
    
    //char* read_file(std::string s);
    char* read_file(const char* filename, int length);
};

extern Roms roms;