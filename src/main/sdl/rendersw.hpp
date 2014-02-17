/***************************************************************************
    SDL Software Video Rendering.  
    
    Known Bugs:
    - Scanlines don't work when Endian changed?

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "renderbase.hpp"

class RenderSW : public RenderBase
{
public:
    RenderSW();
    ~RenderSW();
    bool init(int src_width, int src_height, 
              int scale,
              int video_mode,
              int scanlines);
    void disable();
    bool start_frame();
    bool finalize_frame();
    void draw_frame(uint16_t* pixels);

private:
    // Scanline pixels
    uint32_t* scan_pixels;

    // Pixel Conversion
    uint32_t* pix;

    // Scale the screen
    int scale_factor;

    void scale( uint32_t* src, int srcwid, int srchgt, 
                uint32_t* dest, int dstwid, int dsthgt);
    
    void scanlines_32bpp(uint32_t* src, const int width, const int height, 
                         uint32_t* dst, int percent, const bool interpolate = true);

    void scalex( uint32_t* src, const int srcwid, const int srchgt, uint32_t* dest, const int scale);
};