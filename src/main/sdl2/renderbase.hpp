#pragma once

#include "../stdint.hpp"
#include "../globals.hpp"

#include <SDL.h>

// Abstract Rendering Class
class RenderBase
{
public:
    RenderBase();

    virtual bool init(int src_width, int src_height, 
                      int scale,
                      int video_mode,
                      int scanlines)          = 0;
    virtual void disable()                    = 0;
    virtual bool start_frame()                = 0;
    virtual bool finalize_frame()             = 0;
    virtual void draw_frame(uint16_t* pixels) = 0;
    void convert_palette(uint32_t adr, uint32_t r, uint32_t g, uint32_t b);
    virtual bool supports_window() { return true; }
    virtual bool supports_vsync() { return false; }

protected:
	SDL_Surface *surface;

    // Palette Lookup
    uint32_t rgb[S16_PALETTE_ENTRIES * 3];    // Extended to hold shadow/hilight colours

    uint32_t *screen_pixels;

    // Original Screen Width & Height
    uint16_t orig_width, orig_height;

    // --------------------------------------------------------------------------------------------
    // Screen setup properties. Example below: 
    // ________________________
    // |  |                |  | <- screen size      (e.g. 1280 x 720)
    // |  |                |  |
    // |  |                |<-|--- destination size (e.g. 1027 x 720) to maintain aspect ratio
    // |  |                |  | 
    // |  |                |  |    source size      (e.g. 320  x 224) System 16 proportions
    // |__|________________|__|
    //
    // --------------------------------------------------------------------------------------------

    // Source texture / pixel array that we are going to manipulate
    int src_width, src_height;

    // Destination window width and height
    int dst_width, dst_height;

    // Screen width and height 
    int scn_width, scn_height;

    // Full-Screen, Stretch, Window
    int video_mode;

    // Scanline density. 0 = Off, 1 = Full
    int scanlines;

    // Screen Scale
    int scale;

    // Offsets (for full-screen mode, where x/y resolution isn't a multiple of the original height)
    uint32_t screen_xoff, screen_yoff;

    // SDL Pixel Format Codes. These differ between platforms.
    uint8_t  Rshift, Gshift, Bshift;
    uint32_t Rmask, Gmask, Bmask;

    bool sdl_screen_size();
};