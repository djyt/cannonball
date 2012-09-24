#pragma once

#include "stdint.hpp"

class asmhelper
{
public:
	asmhelper(void);
	~asmhelper(void);

	static void move16(uint32_t src, uint32_t& dst);
	static void move16(int32_t src, int32_t& dst);
	static void add16(uint32_t src, uint32_t& dst);
	static void add16(int32_t src, int32_t& dst);
	static void sub16(uint32_t src, uint32_t& dst);
	static void sub16(int32_t src, int32_t& dst);
	static void swap32(uint32_t&);
	static void swap32(int32_t&);
	static void clear16(uint32_t&);
};

