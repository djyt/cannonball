#include "asmhelper.hpp"

/***************************************************************************
    Assembler Helper Functions. 

    Used to facilitate 68K to C++ porting process.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

asmhelper::asmhelper(void)
{
}

asmhelper::~asmhelper(void)
{
}

// Move 16 bits, preserve original longs
void asmhelper::move16(uint32_t src, uint32_t& dst)
{
    dst = (dst & 0xFFFF0000) + (src & 0xFFFF);
}

void asmhelper::move16(int32_t src, int32_t& dst)
{
    dst = (dst & 0xFFFF0000) + (src & 0xFFFF);
}

// Add two 16 bit words, preserve original longs
//template<typename T>
void asmhelper::add16(uint32_t src, uint32_t& dst)
{
    dst = (dst & 0xFFFF0000) + (((dst & 0xFFFF) + (src & 0xFFFF)) & 0xFFFF);
}

void asmhelper::add16(int32_t src, int32_t& dst)
{
    dst = (dst & 0xFFFF0000) + (((dst & 0xFFFF) + (src & 0xFFFF)) & 0xFFFF);
}

void asmhelper::sub16(uint32_t src, uint32_t& dst)
{
    dst = (dst & 0xFFFF0000) + (((dst & 0xFFFF) - (src & 0xFFFF)) & 0xFFFF);
}

void asmhelper::sub16(int32_t src, int32_t& dst)
{
    dst = (dst & 0xFFFF0000) + (((dst & 0xFFFF) - (src & 0xFFFF)) & 0xFFFF);
}

void asmhelper::swap32(uint32_t& v)
{
    v = ((v & 0xFFFF0000) >> 16) + ((v & 0xFFFF) << 16);
}

void asmhelper::swap32(int32_t& v)
{
    v = ((v & 0xFFFF0000) >> 16) + ((v & 0xFFFF) << 16);
}

void asmhelper::clear16(uint32_t& v)
{
    v = (v & 0xFFFF0000);
}
