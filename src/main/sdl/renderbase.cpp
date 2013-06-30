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
        const SDL_VideoInfo* info = SDL_GetVideoInfo();

        if (!info)
        {
            std::cerr << "Video query failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        orig_width  = info->current_w; 
        orig_height = info->current_h;
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

    r = r * 255 / 31;
    g = g * 255 / 31;
    b = b * 255 / 31;

    rgb[adr] = CURRENT_RGB();
      
    // Create shadow / highlight colours at end of RGB array
    // The resultant values are the same as MAME
    r = r * 202 / 256;
    g = g * 202 / 256;
    b = b * 202 / 256;
        
    rgb[adr + S16_PALETTE_ENTRIES] =
    rgb[adr + (S16_PALETTE_ENTRIES * 2)] = CURRENT_RGB();
}