/***************************************************************************
    Binary File Loader. 
    
    Handles loading an individual binary file to memory.
    Supports reading bytes, words and longs from this area of memory.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "stdint.hpp"
#include "romloader.hpp"

#include <cstddef> // NULL

RomLoader::RomLoader()
{

}

RomLoader::~RomLoader()
{
}

void RomLoader::init(const uint32_t length)
{
    this->length = length;
    rom = new uint8_t[length];
}

void RomLoader::unload(void)
{
    delete[] rom;
}

int RomLoader::load(const char* buffer, const int offset, const int length, const int expected_crc, const uint8_t interleave)
{
    if (buffer == NULL)
        return 1;

    // Interleave file as necessary
    for (int i = 0; i < length; i++)
    {
        rom[(i * interleave) + offset] = buffer[i];
    }

    // Clean Up
    delete[] buffer;
    return 0; // success
}