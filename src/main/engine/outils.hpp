#pragma once

#include "stdint.hpp"
#include "asmhelper.hpp"

class outils
{
public:
    static const uint8_t DEC_TO_HEX[];

	outils();
	~outils();

	static uint32_t random();
	static int32_t isqrt(int32_t);
    static uint16_t convert_speed(uint16_t);
    static uint32_t bcd_add(uint32_t, uint32_t);
    static uint32_t bcd_sub(uint32_t, uint32_t);

private:
	static int32_t next(int32_t, int32_t);
	static int32_t abs(int32_t);
};

