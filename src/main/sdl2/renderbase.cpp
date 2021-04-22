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

void RenderBase::convert_palette(uint32_t adr, uint32_t r1, uint32_t g1, uint32_t b1)
{
    adr >>= 1;

    uint32_t r = r1 * 8;
    uint32_t g = g1 * 8;
    uint32_t b = b1 * 8;

    rgb[adr] = CURRENT_RGB();

    // Create shadow colours at end of RGB array
    r = r1 * shadow_multi / 31;
    g = g1 * shadow_multi / 31;
    b = b1 * shadow_multi / 31;
        
    rgb[adr + S16_PALETTE_ENTRIES] = CURRENT_RGB(); // Add to the end of the array

    // Highlight colour code would be added here, but unused.
}

void RenderBase::set_shadow_intensity(float f)
{
    shadow_multi = (int) std::round(255.0f * f);
}
