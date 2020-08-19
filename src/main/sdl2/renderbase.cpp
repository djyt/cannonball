#include "renderbase.hpp"
#include <iostream>

RenderBase::RenderBase()
{
    surface       = NULL;
    screen_pixels = NULL;

    orig_width  = 0;
    orig_height = 0;
}

// Setup screen size
bool RenderBase::sdl_screen_size()
{
    if (orig_width == 0 || orig_height == 0)
    {
	SDL_DisplayMode info;

	SDL_GetCurrentDisplayMode(0, &info);
        
        orig_width  = info.w; 
        orig_height = info.h;
    }

    scn_width  = orig_width;
    scn_height = orig_height;

    return true;
}

// See: SDL_PixelFormat
#define CURRENT_RGB() (r << Rshift) | (g << Gshift) | (b << Bshift);

void RenderBase::convert_palette(uint32_t adr, uint32_t r, uint32_t g, uint32_t b)
{
    adr >>= 1;

    r = r << 3; //* 255 / 31;
    g = g << 3; //* 255 / 31;
    b = b << 3; //* 255 / 31;

    rgb[adr] = CURRENT_RGB();

    // Create shadow / highlight colours at end of RGB array

    r = r << 2; // * 202 / 256;
    g = g << 2; //* 202 / 256;
    b = b << 2; //* 202 / 256;
    rgb[adr + (S16_PALETTE_ENTRIES << 0)] = CURRENT_RGB(); // highlight

    // The resultant values are the same as MAME
    r = r >> 3; // * 202 / 256;
    g = g >> 3; //* 202 / 256;
    b = b >> 3; //* 202 / 256;

    rgb[adr + (S16_PALETTE_ENTRIES << 1)] = CURRENT_RGB(); // shadow

}
