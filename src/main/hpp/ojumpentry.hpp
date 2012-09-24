#pragma once

#include "stdint.hpp"

//#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

//typedef void (Fred::*FredMemFn)(char x, float y);

struct ojumpentry
{
	// Is jump entry enabled?
	bool enabled;

	// Pointer to function
	//void* (void::*) f(void);

	void* f(void);

	void init(void)
	{
		enabled = false;
	}
}