/***************************************************************************
    SDL Software Video Rendering.  
    
    Known Bugs:
    - Scanlines don't work when Endian changed?

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <iostream>

#include "rendersw.hpp"
#include "frontend/config.hpp"

RenderSW::RenderSW()
{
    scan_pixels = NULL;
}

RenderSW::~RenderSW()
{
    if (scan_pixels) 
        delete[] scan_pixels;
}

bool RenderSW::init(int src_width, int src_height, 
                    int scale,
                    int video_mode,
                    int scanlines)
{
    this->src_width  = src_width;
    this->src_height = src_height;
    this->video_mode = video_mode;
    this->scanlines  = scanlines;

    // Setup SDL Screen size
    if (!RenderBase::sdl_screen_size())
        return false;

    int flags = SDL_FLAGS;

    // --------------------------------------------------------------------------------------------
    // Full Screen Mode
    // When using full-screen mode, we attempt to keep the current resolution.
    // This is because for LCD monitors, I suspect it's what we want to remain in
    // and we don't want to risk upsetting the aspect ratio.
    // --------------------------------------------------------------------------------------------
    if (video_mode == video_settings_t::MODE_FULL || video_mode == video_settings_t::MODE_STRETCH)
    {
        scn_width  = orig_width;
        scn_height = orig_height;

        scale_factor = 0; // Use scaling code
        
        if (video_mode == video_settings_t::MODE_STRETCH)
        {
            dst_width  = scn_width;
            dst_height = scn_height;
            scanlines = 0; // Disable scanlines in stretch mode
        }
        else
        {
            // With scanlines, only allow a proportional scale
            if (scanlines)
            {
                scale_factor = std::min(scn_width / src_width, scn_height / src_height);
                dst_width    = src_width  * scale_factor;
                dst_height   = src_height * scale_factor;
            }
            else
            {
                // Calculate how much to scale screen from its original resolution
                uint32_t w = (scn_width  << 16)  / src_width;
                uint32_t h = (scn_height << 16)  / src_height;
                dst_width  = (src_width  * std::min(w, h)) >> 16;
                dst_height = (src_height * std::min(w, h)) >> 16;
            }
        }
        flags |= SDL_FULLSCREEN; // Set SDL flag
        SDL_ShowCursor(false);   // Don't show mouse cursor in full-screen mode
    }
    // --------------------------------------------------------------------------------------------
    // Windowed Mode
    // --------------------------------------------------------------------------------------------
    else
    {
        this->video_mode = video_settings_t::MODE_WINDOW;
       
        scale_factor  = scale;

        scn_width  = src_width  * scale_factor;
        scn_height = src_height * scale_factor;

        // As we're windowed this is just the same
        dst_width  = scn_width;
        dst_height = scn_height;
        
        SDL_ShowCursor(true);
    }

    // If we're not stretching the screen, centre the image
    if (video_mode != video_settings_t::MODE_STRETCH)
    {
        screen_xoff = scn_width - dst_width;
        if (screen_xoff)
            screen_xoff = (screen_xoff / 2);

        screen_yoff = scn_height - dst_height;
        if (screen_yoff) 
            screen_yoff = (screen_yoff / 2) * scn_width;
    }
    // Otherwise set to the top-left corner
    else
    {
        screen_xoff = 0;
        screen_yoff = 0;
    }

    //int bpp = info->vfmt->BitsPerPixel;
    const int bpp = 32;
    const int available = SDL_VideoModeOK(scn_width, scn_height, bpp, flags);

    // Frees (Deletes) existing surface
    if (surface)
        SDL_FreeSurface(surface);

    // Set the video mode
    surface = SDL_SetVideoMode(scn_width, scn_height, bpp, flags);

    if (!surface || !available)
    {
        std::cerr << "Video mode set failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Convert the SDL pixel surface to 32 bit.
    // This is potentially a larger surface area than the internal pixel array.
    screen_pixels = (uint32_t*)surface->pixels;
    
    // SDL Pixel Format Information
    Rshift = surface->format->Rshift;
    Gshift = surface->format->Gshift;
    Bshift = surface->format->Bshift;
    Rmask  = surface->format->Rmask;
    Gmask  = surface->format->Gmask;
    Bmask  = surface->format->Bmask;

    // Doubled intermediate pixel array for scanlines
    if (scanlines)
    {
        if (scan_pixels) delete[] scan_pixels;
        scan_pixels = new uint32_t[(src_width * 2) * (src_height * 2)];
    }

    return true;
}

void RenderSW::disable()
{

}

bool RenderSW::start_frame()
{
    return !(SDL_MUSTLOCK(surface) && SDL_LockSurface(surface) < 0);
}

bool RenderSW::finalize_frame()
{
    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);

    SDL_Flip(surface);

    return true;
}

void RenderSW::draw_frame(uint32_t* pixels)
{
    // Do Scaling
    if (scale_factor != 1)
    {
        uint32_t* pix = pixels;
    
        // Lookup real RGB value from rgb array for backbuffer
        for (int i = 0; i < (src_width * src_height); i++)
            *(pix++) = rgb[*pix & ((S16_PALETTE_ENTRIES * 3) - 1)];

        // Scanlines: (Full Screen or Windowed). Potentially slow. 
        if (scanlines)
        {
            // Add the scanlines. Double image in the process to create space for the scanlines.
            scanlines_32bpp(pixels, src_width, src_height, scan_pixels, scanlines);

            // Now scale up again
            scale(scan_pixels, src_width * 2, src_height * 2, 
                    screen_pixels + screen_xoff + screen_yoff, dst_width, dst_height);
        }
        // Windowed: Use Faster Scaling algorithm
        else if (video_mode == video_settings_t::MODE_WINDOW)
        {
            scalex(pixels, src_width, src_height, screen_pixels, scale_factor); 
        }
        // Full Screen: Stretch screen. May not be an integer multiple of original size.
        //                              Therefore, scaling is slower.
        else
        {
            scale(pixels, src_width, src_height, 
                    screen_pixels + screen_xoff + screen_yoff, dst_width, dst_height);
        }
    }
    // No Scaling
    else
    {
        uint32_t* pix  = pixels;
        uint32_t* spix = screen_pixels;
    
        // Lookup real RGB value from rgb array for backbuffer
        for (int i = 0; i < (src_width * src_height); i++)
            *(spix++) = rgb[*(pix++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
    }

    // Example: Set the pixel at 10,10 to red
    //pixels[( 10 * surface->w ) + 10] = 0xFF0000;
    // ------------------------------------------------------------------------      
}

// Fastest scaling algorithm. Scales proportionally.
void RenderSW::scalex(uint32_t* src, const int srcwid, const int srchgt, uint32_t* dest, const int scale)
{
    const int destwid = srcwid * scale;

    for (int y = 0; y < srchgt; y++)
    {
        int src_inc = 0;
    
        // First Row
        for (int x = 0; x < destwid; x++)
        {
            *dest++ = *src;
            if (++src_inc == scale)
            {
                src_inc = 0;
                src++;
            }
        }
        // Make additional copies of this row
        for (int i = 0; i < scale-1; i++)
        {
            memcpy(dest, dest - destwid, destwid * sizeof(uint32_t)); 
            dest += destwid;
        }
    }
}

/**
* 
* Fixed point image scaling code (16.16)
* 
* Speed increases when scaling smaller images. Scaling images up is expensive.
* 
* src          pointer to the image we want to scale
* srcwid       how wide is the entire source image?
* srchgt       how tall is the entire source image?
* dest         pointer to the bitmap we want to scale into (destination)
* dstwid       how wide do we want the source image to be?
* dsthgt       how tall do we want the source image to be?
* 
* @author Chris White
* 
*/
void RenderSW::scale( uint32_t* src, int srcwid, int srchgt, 
                      uint32_t* dst, int dstwid, int dsthgt)
{
    int xstep = (srcwid << 16) / dstwid; // calculate distance (in source) between
    int ystep = (srchgt << 16) / dsthgt; // pixels (in dest)
    int srcy  = 0; // y-cordinate in source image
        
    for (int y = 0; y < dsthgt; y++)
    {
        int srcx = 0; // reset our x counter before each row...

        for (int x = 0; x < dstwid; x++)
        {
            *dst++ = *(src + (srcx >> 16));  // copy next pixel
            srcx += xstep;                   // move through source image
        }

        // Ensure we wrap to the next line correctly, when destination screen size
        // is different aspect ratio (e.g. wider)
        if (scn_width > dstwid)
            dst += (scn_width - dstwid);
            
        //
        // figure out if we are still on the same row as last time
        // through the loop, and if so we move the source pointer accordingly.
        // If not, we add nothing to the source counter, and go back to the beginning
        // of the row (in the source image) we just drew.
        //

        srcy += ystep;                  // move through the source image...
        src += ((srcy >> 16) * srcwid); // and possibly to the next row.
        srcy &= 0xffff;                 // set up the y-coordinate between 0 and 1
    }
}

/*****************************************************************************
 ** Original Source: /cvsroot/bluemsx/blueMSX/Src/VideoRender/VideoRender.c,v 
 **
 ** Original Revision: 1.25 
 **
 ** Original Date: 2006/01/17 08:49:34 
 **
 ** More info: http://www.bluemsx.com
 **
 ** Copyright (C) 2003-2004 Daniel Vik
 **
 **  This software is provided 'as-is', without any express or implied
 **  warranty.  In no event will the authors be held liable for any damages
 **  arising from the use of this software.
 **
 **  Permission is granted to anyone to use this software for any purpose,
 **  including commercial applications, and to alter it and redistribute it
 **  freely, subject to the following restrictions:
 **
 **  1. The origin of this software must not be misrepresented; you must not
 **     claim that you wrote the original software. If you use this software
 **     in a product, an acknowledgment in the product documentation would be
 **     appreciated but is not required.
 **  2. Altered source versions must be plainly marked as such, and must not be
 **     misrepresented as being the original software.
 **  3. This notice may not be removed or altered from any source distribution.
 **
 ******************************************************************************
  */

// Modified version of the original to handle different 32bpp formats.

// Note that this takes an unscaled source pixel array as input, and then 
// doubles it up, in order to insert the scanlines.

void RenderSW::scanlines_32bpp(uint32_t* src, const int width, const int height, 
                            uint32_t* dst, int percent, bool interpolate)
{
    const int dst_width1 = width << 1;
    const int dst_width2 = width << 2;
    
    uint32_t* sBuf = dst;              // Normal Scanline
    uint32_t* pBuf = dst + dst_width1; // Darkened Scanline
    
    for (int y = 0; y < height; y++)
    {
        // Double the pixels for this row
        for (int x = 0; x < width; x++)
        {
            *dst++ = *src;
            *dst++ = *src++;
        }

        // Omit one row.
        // We will fill the omitted row with a scanline later
        dst += dst_width1;
    }
        
    // Optimization for black scanlines
    if (percent == 100)
    {
        for (int h = 0; h < height; h++) 
        {
            memset(pBuf, 0, dst_width1 * sizeof(uint32_t));
            pBuf += dst_width2; // Advance two lines
        }
        return;
    }

    if (interpolate) 
    {
        uint32_t* tBuf = sBuf + dst_width2;    // Next Scanline (For Interpolation)
        int percent_orig = percent;            // Backup for final scanline

        percent = ((100-percent) << 8) / 200;
        for (int h = 0; h < height-1; h++) 
        {
            for (int w = 0; w < dst_width1; w++) 
            {
                uint32_t pixel1 = sBuf[w]; // Normal Pixel
                uint32_t pixel2 = tBuf[w]; // Pixel to interpolate
                uint32_t r = (( ((pixel1 & Rmask)+(pixel2 & Rmask)) * percent) >> 8) & Rmask;
                uint32_t g = (( ((pixel1 & Gmask)+(pixel2 & Gmask)) * percent) >> 8) & Gmask;
                uint32_t b = (( ((pixel1 & Bmask)+(pixel2 & Bmask)) * percent) >> 8) & Bmask;
                pBuf[w] = r | g | b;
            }
            sBuf += dst_width2; // Advance two lines
            tBuf += dst_width2;
            pBuf += dst_width2;
        }

        // Do final scanline (no interpolation with next line)
        percent = ((100-percent_orig) << 8) / 100;
        for (int w = 0; w < dst_width1; w++) 
        {
            uint32_t pixel = sBuf[w];
            uint32_t r = (( (pixel & Rmask) * percent) >> 8) & Rmask;
            uint32_t g = (( (pixel & Gmask) * percent) >> 8) & Gmask;
            uint32_t b = (( (pixel & Bmask) * percent) >> 8) & Bmask;
            pBuf[w] = r | g | b;
        }
    } 
    else 
    {
        percent = ((100-percent) << 8) / 100;
        for (int h = 0; h < height; h++) 
        {
            for (int w = 0; w < dst_width1; w++) 
            {
                uint32_t pixel = sBuf[w];
                uint32_t r = (( (pixel & Rmask) * percent) >> 8) & Rmask;
                uint32_t g = (( (pixel & Gmask) * percent) >> 8) & Gmask;
                uint32_t b = (( (pixel & Bmask) * percent) >> 8) & Bmask;
                pBuf[w] = r | g | b;
            }
            sBuf += dst_width2; // Advance two lines
            pBuf += dst_width2;
        }
    }
}