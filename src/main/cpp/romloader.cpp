/**
   1. ifstream - open the file for input
   2. ofstream - open the file for output
   3. fstream - open the file for input/output/both

   http://www.cplusplus.com/doc/tutorial/files/

   Note default path will be Visual Studio 2010\Projects\outrun\outrun

*/

#include "romloader.hpp"

RomLoader::RomLoader()
{

}

RomLoader::~RomLoader()
{
	//delete[] rom;
}

void RomLoader::init(const uint32_t length)
{
	rom = new uint8_t[length];
}

int RomLoader::load(const char* filename, const int offset, const int length, const uint8_t interleave)
{
	// Open rom file
	std::ifstream src(filename, std::ios::in | std::ios::binary);
	if (!src)
	{
		error("cannot open rom:", filename);
		return 0; // fail
	}

	char ch;

	for (int i = 0; i < length; i++)
	{
		src.get(ch); // Get from source
		rom[(i * interleave) + offset] = ch;
	}

	src.close();
	return 1; // success
}

void RomLoader::error(const char* p, const char* p2)
{
	std::cout << p << ' ' << p2 << std::endl;
}

void RomLoader::unload(void)
{
	delete[] rom;
}

uint32_t RomLoader::read32(uint32_t* addr)
{	
	//The following returns 4 bytes, but in reverse order.
	//char* p = &rom0[addr];
	//return *( reinterpret_cast<uint32_t *>(p) );
	uint32_t data = (rom[*addr] << 24) | (rom[*addr+1] << 16) | (rom[*addr+2] << 8) | (rom[*addr+3]);
	*addr += 4;
	return data;
}

uint16_t RomLoader::read16(uint32_t* addr)
{
	uint16_t data = (rom[*addr] << 8) | (rom[*addr+1]);
	*addr += 2;
	return data;
}

uint8_t RomLoader::read8(uint32_t* addr)
{
    return rom[(*addr)++]; 
}

uint32_t RomLoader::read32(uint32_t addr)
{	
	return (rom[addr] << 24) | (rom[addr+1] << 16) | (rom[addr+2] << 8) | rom[addr+3];
}

uint16_t RomLoader::read16(uint32_t addr)
{
	return (rom[addr] << 8) | rom[addr+1];
}

uint8_t RomLoader::read8(uint32_t addr)
{
    return rom[addr];
}