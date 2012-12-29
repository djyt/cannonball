/***************************************************************************
    OutRun Utility Functions & Assembler Helper Functions. 

    Common OutRun library functions.
    Helper functions used to facilitate 68K to C++ porting process.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "stdint.hpp"
#include "frontend/config.hpp"

class outils
{
public:
    static const uint8_t DEC_TO_HEX[];

	outils();
	~outils();

    static void reset_random_seed();
	static uint32_t random();
	static int32_t isqrt(int32_t);
    static uint16_t convert_speed(uint16_t);
    static uint32_t bcd_add(uint32_t, uint32_t);
    static uint32_t bcd_sub(uint32_t, uint32_t);

    // Inline functions
    inline static void move16(uint32_t src, uint32_t& dst)
    {
        dst = (dst & 0xFFFF0000) + (src & 0xFFFF);
    }

    inline static void add16(uint32_t src, uint32_t& dst)
    {
        dst = (dst & 0xFFFF0000) + (((dst & 0xFFFF) + (src & 0xFFFF)) & 0xFFFF);
    }

    inline static void sub16(int32_t src, int32_t& dst)
    {
        dst = (dst & 0xFFFF0000) + (((dst & 0xFFFF) - (src & 0xFFFF)) & 0xFFFF);
    }

    inline static void swap32(int32_t& v)
    {
        v = ((v & 0xFFFF0000) >> 16) + ((v & 0xFFFF) << 16);
    }

    inline static void swap32(uint32_t& v)
    {
        v = ((v & 0xFFFF0000) >> 16) + ((v & 0xFFFF) << 16);
    }

private:
	static int32_t next(int32_t, int32_t);
	static int32_t abs(int32_t);
};