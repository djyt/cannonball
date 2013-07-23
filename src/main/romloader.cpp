/***************************************************************************
    Binary File Loader. 
    
    Handles loading an individual binary file to memory.
    Supports reading bytes, words and longs from this area of memory.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <iostream>
#include <fstream>
#include <cstddef>       // for std::size_t
//#include <boost/crc.hpp> // CRC Checking via Boost library.

#include "stdint.hpp"
#include "romloader.hpp"

#ifdef __APPLE__
#include "CoreFoundation/CoreFoundation.h"
#endif

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

int RomLoader::load(const char* filename, const int offset, const int length, const int expected_crc, const uint8_t interleave)
{

#ifdef __APPLE__    
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char bundlepath[PATH_MAX];

    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)bundlepath, PATH_MAX))
    {
        // error!
    }

    CFRelease(resourcesURL);
    chdir(bundlepath);
#endif

    std::string path = "roms/";
    path += std::string(filename);

    // Open rom file
    std::ifstream src(path.c_str(), std::ios::in | std::ios::binary);
    if (!src)
    {
        std::cout << "cannot open rom: " << filename << std::endl;
        return 1; // fail
    }

    // Read file
    char* buffer = new char[length];
    src.read(buffer, length);

    // Check CRC on file
    // HACKED OUT
    /*boost::crc_32_type result;
    result.process_bytes(buffer, (size_t) src.gcount());

    if (expected_crc != result.checksum())
    {
        std::cout << std::hex << 
            filename << " has incorrect checksum.\nExpected: " << expected_crc << " Found: " << result.checksum() << std::endl;
    }*/

    // Interleave file as necessary
    for (int i = 0; i < length; i++)
    {
        rom[(i * interleave) + offset] = buffer[i];
    }

    // Clean Up
    delete[] buffer;
    src.close();
    return 0; // success
}