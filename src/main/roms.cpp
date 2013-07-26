/***************************************************************************
    Load OutRun ROM Set.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <iostream>
#include <fstream>
#include <cstddef>       // for std::size_t

#include <cstring>
//#include <boost/crc.hpp> // CRC Checking via Boost library.

#include "stdint.hpp"
#include "roms.hpp"

Roms roms;

Roms::Roms()
{
    zip_file = NULL;
    jap_rom_status = -1;
}

Roms::~Roms(void)
{
}

bool Roms::open_zip(const char* filename)
{
    zip_file = unzOpen(filename);
    if (zip_file == NULL)
    {
        std::cout << "Error opening zip file " << filename << std::endl;
    }
    
    return (zip_file != NULL);
}

void Roms::close_zip()
{
    if (zip_file != NULL)
    {
        unzClose(zip_file);
        zip_file = NULL;
    }
}

char* Roms::read_file(const char* filename, int length)
{
    if (zip_file == NULL)
    {
        std::string path = "roms/";
        path += std::string(filename);

        // Open rom file
        std::ifstream src(path.c_str(), std::ios::in | std::ios::binary);
        if (!src)
        {
            std::cout << "cannot open rom: " << filename << std::endl;
            return NULL; // fail
        }

        // Read file
        char* buffer = new char[length];
        src.read(buffer, length);
        src.close();
        return buffer;
    }
    else
    {
        //std::cout << "Loading rom from zip: " << filename << std::endl;
    
        // Ignore case sensitivity (2)
        if (unzLocateFile(zip_file, filename, 2) != UNZ_OK)
        {
            std::cout << "cant locate zipped rom: " << filename << std::endl;
            close_zip();
            return NULL;
        }
        
        unz_file_info info;
        if (unzGetCurrentFileInfo(zip_file, &info, NULL, 0, NULL, 0, NULL, 0 ) != UNZ_OK)
        {
            std::cout << "Error getting info from file: " << filename << std::endl;
            close_zip();
            return NULL;       
        }
        
        if (unzOpenCurrentFile(zip_file) != UNZ_OK)
        {
            std::cout << "Error opening zipped rom: " << filename << std::endl;
            close_zip();
            return NULL;            
        }
        
        if (info.uncompressed_size == length)
        {
            char* buffer = new char[info.uncompressed_size];
            int size_read = unzReadCurrentFile(zip_file, buffer, info.uncompressed_size);
            return buffer;
        }
        else
        {
            std::cout << "Zipped rom has wrong file size: " << filename << " (" << info.uncompressed_size << ")" << std::endl;
            close_zip();
        }      
        return NULL;
    }
}

bool Roms::load_revb_roms()
{
    // If incremented, a rom has failed to load.
    int status = 0;

    // Load Master CPU ROMs
    rom0.init(0x40000);
    status += rom0.load(read_file("epr-10381a.132", 0x10000), 0x20000, 0x10000, 0xbe8c412b, RomLoader::INTERLEAVE2);
    
    // Try alternate filename for this rom
    if (status)
    {
        rom0.load(read_file("epr-10381b.132", 0x10000), 0x20000, 0x10000, 0xbe8c412b, RomLoader::INTERLEAVE2);
        status--;
    }
    
    status += rom0.load(read_file("epr-10383b.117", 0x10000), 0x20001, 0x10000, 0x10a2014a, RomLoader::INTERLEAVE2);
    status += rom0.load(read_file("epr-10380b.133", 0x10000), 0x00000, 0x10000, 0x1f6cadad, RomLoader::INTERLEAVE2);
    status += rom0.load(read_file("epr-10382b.118", 0x10000), 0x00001, 0x10000, 0xc4c3fa1a, RomLoader::INTERLEAVE2);

    // Load Slave CPU ROMs
    rom1.init(0x40000);
    status += rom1.load(read_file("epr-10327a.76", 0x10000), 0x00000, 0x10000, 0xe28a5baf, RomLoader::INTERLEAVE2);
    status += rom1.load(read_file("epr-10329a.58", 0x10000), 0x00001, 0x10000, 0xda131c81, RomLoader::INTERLEAVE2);
    status += rom1.load(read_file("epr-10328a.75", 0x10000), 0x20000, 0x10000, 0xd5ec5e5d, RomLoader::INTERLEAVE2);
    status += rom1.load(read_file("epr-10330a.57", 0x10000), 0x20001, 0x10000, 0xba9ec82a, RomLoader::INTERLEAVE2);

    // Load Non-Interleaved Tile ROMs
    tiles.init(0x30000);
    status += tiles.load(read_file("opr-10268.99", 0x08000),  0x00000, 0x08000, 0x95344b04);
    status += tiles.load(read_file("opr-10232.102", 0x08000), 0x08000, 0x08000, 0x776ba1eb);
    status += tiles.load(read_file("opr-10267.100", 0x08000), 0x10000, 0x08000, 0xa85bb823);
    status += tiles.load(read_file("opr-10231.103", 0x08000), 0x18000, 0x08000, 0x8908bcbf);
    status += tiles.load(read_file("opr-10266.101", 0x08000), 0x20000, 0x08000, 0x9f6f1a74);
    status += tiles.load(read_file("opr-10230.104", 0x08000), 0x28000, 0x08000, 0x686f5e50);

    // Load Non-Interleaved Road ROMs (2 identical roms, 1 for each road)
    road.init(0x10000);
    status += road.load(read_file("opr-10185.11", 0x08000), 0x000000, 0x08000, 0x22794426);
    status += road.load(read_file("opr-10186.47", 0x08000), 0x008000, 0x08000, 0x22794426);

    // Load Interleaved Sprite ROMs
    sprites.init(0x100000);
    status += sprites.load(read_file("mpr-10371.9", 0x20000),  0x000000, 0x20000, 0x7cc86208, RomLoader::INTERLEAVE4);
    status += sprites.load(read_file("mpr-10373.10", 0x20000), 0x000001, 0x20000, 0xb0d26ac9, RomLoader::INTERLEAVE4);
    status += sprites.load(read_file("mpr-10375.11", 0x20000), 0x000002, 0x20000, 0x59b60bd7, RomLoader::INTERLEAVE4);
    status += sprites.load(read_file("mpr-10377.12", 0x20000), 0x000003, 0x20000, 0x17a1b04a, RomLoader::INTERLEAVE4);
    status += sprites.load(read_file("mpr-10372.13", 0x20000), 0x080000, 0x20000, 0xb557078c, RomLoader::INTERLEAVE4);
    status += sprites.load(read_file("mpr-10374.14", 0x20000), 0x080001, 0x20000, 0x8051e517, RomLoader::INTERLEAVE4);
    status += sprites.load(read_file("mpr-10376.15", 0x20000), 0x080002, 0x20000, 0xf3b8f318, RomLoader::INTERLEAVE4);
    status += sprites.load(read_file("mpr-10378.16", 0x20000), 0x080003, 0x20000, 0xa1062984, RomLoader::INTERLEAVE4);

    // Load Z80 Sound ROM
    z80.init(0x10000);
    status += z80.load(read_file("epr-10187.88", 0x08000), 0x0000, 0x08000, 0xa10abaa9);

    // Load Sega PCM Chip Samples
    pcm.init(0x60000);
    status += pcm.load(read_file("opr-10193.66", 0x08000), 0x00000, 0x08000, 0xbcd10dde);
    status += pcm.load(read_file("opr-10192.67", 0x08000), 0x10000, 0x08000, 0x770f1270);
    status += pcm.load(read_file("opr-10191.68", 0x08000), 0x20000, 0x08000, 0x20a284ab);
    status += pcm.load(read_file("opr-10190.69", 0x08000), 0x30000, 0x08000, 0x7cab70e2);
    status += pcm.load(read_file("opr-10189.70", 0x08000), 0x40000, 0x08000, 0x01366b54);
    status += pcm.load(read_file("opr-10188.71", 0x08000), 0x50000, 0x08000, 0xbad30ad9);

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
    jap_rom_status += j_rom0.load(read_file("epr-10380.133", 0x10000), 0x00000, 0x10000, 0xe339e87a, RomLoader::INTERLEAVE2);
    jap_rom_status += j_rom0.load(read_file("epr-10382.118", 0x10000), 0x00001, 0x10000, 0x65248dd5, RomLoader::INTERLEAVE2);
    jap_rom_status += j_rom0.load(read_file("epr-10381.132", 0x10000), 0x20000, 0x10000, 0xbe8c412b, RomLoader::INTERLEAVE2);
    jap_rom_status += j_rom0.load(read_file("epr-10383.117", 0x10000), 0x20001, 0x10000, 0xdcc586e7, RomLoader::INTERLEAVE2);

    // Load Slave CPU ROMs        
    jap_rom_status += j_rom1.load(read_file("epr-10327.76", 0x10000), 0x00000, 0x10000, 0xda99d855, RomLoader::INTERLEAVE2);
    jap_rom_status += j_rom1.load(read_file("epr-10329.58", 0x10000), 0x00001, 0x10000, 0xfe0fa5e2, RomLoader::INTERLEAVE2);
    jap_rom_status += j_rom1.load(read_file("epr-10328.75", 0x10000), 0x20000, 0x10000, 0x3c0e9a7f, RomLoader::INTERLEAVE2);
    jap_rom_status += j_rom1.load(read_file("epr-10330.57", 0x10000), 0x20001, 0x10000, 0x59786e99, RomLoader::INTERLEAVE2);
    // If status has been incremented, a rom has failed to load.
    return jap_rom_status == 0;
}