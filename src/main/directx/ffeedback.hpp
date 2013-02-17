#pragma once

//-----------------------------------------------------------------------------
// Function prototypes 
//-----------------------------------------------------------------------------
namespace forcefeedback
{
    extern void init();
    extern void close();
    extern int  set(int xdirection, int force);
};