#include <iostream>

#include "sdl/video.hpp"
#include "setup.hpp"
#include "globals.hpp"
#include "frontend/config.hpp"
    
Video video;

Video::Video(void)
{
    orig_width  = 0;
    orig_height = 0;
    pixels      = NULL;
    scan_pixels = NULL;
    surface     = NULL;
    sprite_layer = new hwsprites();
    tile_layer = new hwtiles();
}

Video::~Video(void)
{
    delete sprite_layer;
    delete tile_layer;
    if (pixels) delete[] pixels;
    if (scanlines) delete[] scan_pixels;
}

int Video::init(Roms* roms, video_settings_t* settings)
{
    if (orig_width == 0 || orig_height == 0)
    {
        const SDL_VideoInfo* info = SDL_GetVideoInfo();

        if (!info)
        {
            std::cerr << "Video query failed: " << SDL_GetError() << std::endl;
            return 0;
        }
        
        orig_width  = info->current_w; 
        orig_height = info->current_h;
    }

    if (!set_video_mode(settings))
        return 0;

    // Internal pixel array. The size of this is always constant
    if (pixels) delete[] pixels;
    pixels = new uint32_t[config.s16_width * config.s16_height];

    // Doubled intermediate pixel array for scanlines
    if (scanlines)
    {
        if (scan_pixels) delete[] scan_pixels;
        scan_pixels = new uint32_t[(config.s16_width * 2) * (config.s16_height * 2)];
    }

    // Convert S16 tiles to a more useable format
    tile_layer->init(roms->tiles.rom, config.video.hires != 0);
    clear_tile_ram();
    clear_text_ram();
    if (roms->tiles.rom)
    {
        delete[] roms->tiles.rom;
        roms->tiles.rom = NULL;
    }

    // Convert S16 sprites
    sprite_layer->init(roms->sprites.rom);
    if (roms->sprites.rom)
    {
        delete[] roms->sprites.rom;
        roms->sprites.rom = NULL;
    }

    // Convert S16 Road Stuff
    hwroad.init(roms->road.rom, config.video.hires != 0);
    if (roms->road.rom)
    {
        delete[] roms->road.rom;
        roms->road.rom = NULL;
    }

    enabled = true;
    return 1;
}

// ------------------------------------------------------------------------------------------------
// Set SDL Video Mode From Configuration File
// ------------------------------------------------------------------------------------------------

int Video::set_video_mode(video_settings_t* settings)
{
    if (settings->widescreen)
    {
        config.s16_width  = S16_WIDTH_WIDE;
        config.s16_x_off = (S16_WIDTH_WIDE - S16_WIDTH) / 2;
    }
    else
    {
        config.s16_width = S16_WIDTH;
        config.s16_x_off = 0;
    }

    config.s16_height = S16_HEIGHT;

    // Internal video buffer is doubled in hi-res mode.
    if (settings->hires)
    {
        config.s16_width  <<= 1;
        config.s16_height <<= 1;
    }

    //int bpp = info->vfmt->BitsPerPixel;
    const int bpp = 32;
    int flags = SDL_FLAGS;
    
    scanlines = settings->scanlines;
    if (scanlines < 0) scanlines = 0;
    else if (scanlines > 100) scanlines = 100;

    video_mode = settings->mode;

    // --------------------------------------------------------------------------------------------
    // Full Screen Mode
    // When using full-screen mode, we attempt to keep the current resolution.
    // This is because for LCD monitors, I suspect it's what we want to remain in
    // and we don't want to risk upsetting the aspect ratio.
    // --------------------------------------------------------------------------------------------
    if (video_mode == video_settings_t::MODE_FULL || video_mode == video_settings_t::MODE_STRETCH)
    {
        screen_width  = orig_width;
        screen_height = orig_height;

        scale_factor = 0; // Use scaling code
        
        if (video_mode == video_settings_t::MODE_STRETCH)
        {
            scaled_width  = screen_width;
            scaled_height = screen_height;
            scanlines = 0; // Disable scanlines in stretch mode
        }
        else
        {
            // With scanlines, only allow a proportional scale
            if (scanlines)
            {
                scale_factor  = std::min(screen_width / config.s16_width, screen_height / config.s16_height);
                scaled_width  = config.s16_width  * scale_factor;
                scaled_height = config.s16_height * scale_factor;
            }
            else
            {
                // Calculate how much to scale screen from its original resolution
                uint32_t w = (screen_width << 16)  / config.s16_width;
                uint32_t h = (screen_height << 16) / config.s16_height;
                scaled_width  = (config.s16_width  * std::min(w, h)) >> 16;
                scaled_height = (config.s16_height * std::min(w, h)) >> 16;
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
        video_mode = video_settings_t::MODE_WINDOW;

        if (settings->scale < 1)
            settings->scale = 1;
       
        scale_factor  = settings->scale;

        screen_width  = config.s16_width  * scale_factor;
        screen_height = config.s16_height * scale_factor;

        // As we're windowed this is just the same
        scaled_width  = screen_width;
        scaled_height = screen_height;
        
        SDL_ShowCursor(true);
    }

    // If we're not stretching the screen, centre the image
    if (video_mode != video_settings_t::MODE_STRETCH)
    {
        screen_xoff = screen_width - scaled_width;
        if (screen_xoff)
            screen_xoff = (screen_xoff / 2);

        screen_yoff = screen_height - scaled_height;
        if (screen_yoff) 
            screen_yoff = (screen_yoff / 2) * screen_width;
    }
    // Otherwise set to the top-left corner
    else
    {
        screen_xoff = 0;
        screen_yoff = 0;
    }
   
    int available = SDL_VideoModeOK(screen_width, screen_height, bpp, flags);

    // Frees (Deletes) existing surface
    if (surface)
        SDL_FreeSurface(surface);

    // Set the video mode
    surface = SDL_SetVideoMode(screen_width, screen_height, bpp, flags);

    if (!surface || !available)
    {
        std::cerr << "Video mode set failed: " << SDL_GetError() << std::endl;
        return 0;
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

    return 1;
}

void Video::draw_frame(void)
{
    if (SDL_MUSTLOCK(surface) && SDL_LockSurface(surface) < 0) 
        return;

    if (!enabled)
    {
        SDL_FillRect(surface, NULL, 0);
    }
    else
    {
        // ------------------------------------------------------------------------
        // Draw
        // ------------------------------------------------------------------------
        tile_layer->update_tile_values();

        (hwroad.*hwroad.render_background)(pixels);
        sprite_layer->render(1);
        tile_layer->render_tile_layer(pixels, 1, 0);      // background layer
        sprite_layer->render(2);
        tile_layer->render_tile_layer(pixels, 1, 1);      // background layer
        tile_layer->render_tile_layer(pixels, 0, 0);      // foreground layer
        sprite_layer->render(4);
        tile_layer->render_tile_layer(pixels, 0, 1);      // foreground layer
        (hwroad.*hwroad.render_foreground)(pixels);
        tile_layer->render_text_layer(pixels, 0);
        sprite_layer->render(8);
        tile_layer->render_text_layer(pixels, 1);
 
        // Do Scaling
        if (scale_factor != 1)
        {
            uint32_t* pix = pixels;
    
            // Lookup real RGB value from rgb array for backbuffer
            for (int i = 0; i < (config.s16_width * config.s16_height); i++)    
                *(pix++) = rgb[*pix & ((S16_PALETTE_ENTRIES * 3) - 1)];

            // Scanlines: (Full Screen or Windowed). Potentially slow. 
            if (scanlines)
            {
                // Add the scanlines. Double image in the process to create space for the scanlines.
                scanlines_32bpp(pixels, config.s16_width, config.s16_height, scan_pixels, scanlines);

                // Now scale up again
                scale(scan_pixels, config.s16_width * 2, config.s16_height * 2, 
                      screen_pixels + screen_xoff + screen_yoff, scaled_width, scaled_height);
            }
            // Windowed: Use Faster Scaling algorithm
            else if (video_mode == video_settings_t::MODE_WINDOW)
            {
                scalex(pixels, config.s16_width, config.s16_height, screen_pixels, scale_factor); 
            }
            // Full Screen: Stretch screen. May not be an integer multiple of original size.
            //                              Therefore, scaling is slower.
            else
            {
                scale(pixels, config.s16_width, config.s16_height, 
                      screen_pixels + screen_xoff + screen_yoff, scaled_width, scaled_height);
            }
        }
        // No Scaling
        else
        {
            uint32_t* pix  = pixels;
            uint32_t* spix = screen_pixels;
    
            // Lookup real RGB value from rgb array for backbuffer
            for (int i = 0; i < (config.s16_width * config.s16_height); i++)
                *(spix++) = rgb[*(pix++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        }

        // Example: Set the pixel at 10,10 to red
        //pixels[( 10 * surface->w ) + 10] = 0xFF0000;
        // ------------------------------------------------------------------------
    }
    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);

    SDL_Flip(surface);
}

// Fastest scaling algorithm. Scales proportionally.
void Video::scalex(uint32_t* src, const int srcwid, const int srchgt, uint32_t* dest, const int scale)
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
void Video::scale( uint32_t* src, int srcwid, int srchgt, 
                   uint32_t* dest, int dstwid, int dsthgt)
{
    int xstep = (srcwid << 16) / dstwid; // calculate distance (in source) between
    int ystep = (srchgt << 16) / dsthgt; // pixels (in dest)
    int srcy  = 0; // y-cordinate in source image
        
    for (int y = 0; y < dsthgt; y++)
    {
        int srcx = 0; // reset our x counter before each row...

        for (int x = 0; x < dstwid; x++)
        {
            *dest++ = *(src + (srcx >> 16)); // copy next pixel
            srcx += xstep;                   // move through source image
        }

        // Ensure we wrap to the next line correctly, when destination screen size
        // is different aspect ratio (e.g. wider)
        if (screen_width > dstwid)
            dest += (screen_width - dstwid);
            
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

void Video::scanlines_32bpp(uint32_t* src, const int width, const int height, 
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
            Uint32 pixel = sBuf[w];
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
                Uint32 pixel = sBuf[w];
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

// ---------------------------------------------------------------------------
// Text Handling Code
// ---------------------------------------------------------------------------

void Video::clear_text_ram()
{
    for (uint32_t i = 0; i <= 0xFFF; i++)
        tile_layer->text_ram[i] = 0;
}

void Video::write_text8(uint32_t addr, const uint8_t data)
{
    tile_layer->text_ram[addr & 0xFFF] = data;
}

void Video::write_text16(uint32_t* addr, const uint16_t data)
{
    tile_layer->text_ram[*addr & 0xFFF] = (data >> 8) & 0xFF;
    tile_layer->text_ram[(*addr+1) & 0xFFF] = data & 0xFF;

    *addr += 2;
}

void Video::write_text16(uint32_t addr, const uint16_t data)
{
    tile_layer->text_ram[addr & 0xFFF] = (data >> 8) & 0xFF;
    tile_layer->text_ram[(addr+1) & 0xFFF] = data & 0xFF;
}

void Video::write_text32(uint32_t* addr, const uint32_t data)
{
    tile_layer->text_ram[*addr & 0xFFF] = (data >> 24) & 0xFF;
    tile_layer->text_ram[(*addr+1) & 0xFFF] = (data >> 16) & 0xFF;
    tile_layer->text_ram[(*addr+2) & 0xFFF] = (data >> 8) & 0xFF;
    tile_layer->text_ram[(*addr+3) & 0xFFF] = data & 0xFF;

    *addr += 4;
}

void Video::write_text32(uint32_t addr, const uint32_t data)
{
    tile_layer->text_ram[addr & 0xFFF] = (data >> 24) & 0xFF;
    tile_layer->text_ram[(addr+1) & 0xFFF] = (data >> 16) & 0xFF;
    tile_layer->text_ram[(addr+2) & 0xFFF] = (data >> 8) & 0xFF;
    tile_layer->text_ram[(addr+3) & 0xFFF] = data & 0xFF;
}

uint8_t Video::read_text8(uint32_t addr)
{
    return tile_layer->text_ram[addr & 0xFFF];
}

// ---------------------------------------------------------------------------
// Tile Handling Code
// ---------------------------------------------------------------------------

void Video::clear_tile_ram()
{
    for (uint32_t i = 0; i <= 0xFFFF; i++)
        tile_layer->tile_ram[i] = 0;
}

void Video::write_tile8(uint32_t addr, const uint8_t data)
{
    tile_layer->tile_ram[addr & 0xFFFF] = data;
} 

void Video::write_tile16(uint32_t* addr, const uint16_t data)
{
    tile_layer->tile_ram[*addr & 0xFFFF] = (data >> 8) & 0xFF;
    tile_layer->tile_ram[(*addr+1) & 0xFFFF] = data & 0xFF;

    *addr += 2;
}

void Video::write_tile16(uint32_t addr, const uint16_t data)
{
    tile_layer->tile_ram[addr & 0xFFFF] = (data >> 8) & 0xFF;
    tile_layer->tile_ram[(addr+1) & 0xFFFF] = data & 0xFF;
}   

void Video::write_tile32(uint32_t* addr, const uint32_t data)
{
    tile_layer->tile_ram[*addr & 0xFFFF] = (data >> 24) & 0xFF;
    tile_layer->tile_ram[(*addr+1) & 0xFFFF] = (data >> 16) & 0xFF;
    tile_layer->tile_ram[(*addr+2) & 0xFFFF] = (data >> 8) & 0xFF;
    tile_layer->tile_ram[(*addr+3) & 0xFFFF] = data & 0xFF;

    *addr += 4;
}

void Video::write_tile32(uint32_t addr, const uint32_t data)
{
    tile_layer->tile_ram[addr & 0xFFFF] = (data >> 24) & 0xFF;
    tile_layer->tile_ram[(addr+1) & 0xFFFF] = (data >> 16) & 0xFF;
    tile_layer->tile_ram[(addr+2) & 0xFFFF] = (data >> 8) & 0xFF;
    tile_layer->tile_ram[(addr+3) & 0xFFFF] = data & 0xFF;
}

uint8_t Video::read_tile8(uint32_t addr)
{
    return tile_layer->tile_ram[addr & 0xFFFF];
}


// ---------------------------------------------------------------------------
// Sprite Handling Code
// ---------------------------------------------------------------------------

void Video::write_sprite16(uint32_t* addr, const uint16_t data)
{
    sprite_layer->write(*addr & 0xfff, data);
    *addr += 2;
}

// ---------------------------------------------------------------------------
// Palette Handling Code
// ---------------------------------------------------------------------------

void Video::write_pal8(uint32_t* palAddr, const uint8_t data)
{
    palette[*palAddr & 0x1fff] = data;
    refresh_palette(*palAddr & 0x1fff);
    *palAddr += 1;
}

void Video::write_pal16(uint32_t* palAddr, const uint16_t data)
{    
    uint32_t adr = *palAddr & 0x1fff;
    palette[adr] = (data >> 8) & 0xFF;
    palette[adr+1] = data & 0xFF;
    refresh_palette(adr);
    *palAddr += 2;
}

void Video::write_pal32(uint32_t* palAddr, const uint32_t data)
{    
    uint32_t adr = *palAddr & 0x1fff;

    palette[adr] = (data >> 24) & 0xFF;
    palette[adr+1] = (data >> 16) & 0xFF;
    palette[adr+2] = (data >> 8) & 0xFF;
    palette[adr+3] = data & 0xFF;

    refresh_palette(adr);
    refresh_palette(adr+2);

    *palAddr += 4;
}

void Video::write_pal32(uint32_t adr, const uint32_t data)
{    
    adr &= 0x1fff;

    palette[adr] = (data >> 24) & 0xFF;
    palette[adr+1] = (data >> 16) & 0xFF;
    palette[adr+2] = (data >> 8) & 0xFF;
    palette[adr+3] = data & 0xFF;
    refresh_palette(adr);
    refresh_palette(adr+2);
}

uint8_t Video::read_pal8(uint32_t palAddr)
{
    return palette[palAddr & 0x1fff];
}

uint16_t Video::read_pal16(uint32_t palAddr)
{
    uint32_t adr = palAddr & 0x1fff;
    return (palette[adr] << 8) | palette[adr+1];
}

uint16_t Video::read_pal16(uint32_t* palAddr)
{
    uint32_t adr = *palAddr & 0x1fff;
    *palAddr += 2;
    return (palette[adr] << 8)| palette[adr+1];
}

uint32_t Video::read_pal32(uint32_t* palAddr)
{
    uint32_t adr = *palAddr & 0x1fff;
    *palAddr += 4;
    return (palette[adr] << 24) | (palette[adr+1] << 16) | (palette[adr+2] << 8) | palette[adr+3];
}

// See: SDL_PixelFormat
#define CURRENT_RGB() (r << Rshift) | (g << Gshift) | (b << Bshift);

void Video::refresh_palette(uint32_t palAddr)
{
    palAddr &= ~1;
    uint32_t rgbAddr = palAddr >> 1;
    uint32_t a = (palette[palAddr] << 8) | palette[palAddr + 1];
    uint32_t r = (a & 0x000f) << 1; // r rrr0
    uint32_t g = (a & 0x00f0) >> 3; // g ggg0
    uint32_t b = (a & 0x0f00) >> 7; // b bbb0
    if ((a & 0x1000) != 0)
        r |= 1; // r rrrr
    if ((a & 0x2000) != 0)
        g |= 1; // g gggg
    if ((a & 0x4000) != 0)
        b |= 1; // b bbbb

    r = r * 255 / 31;
    g = g * 255 / 31;
    b = b * 255 / 31;

    rgb[rgbAddr] = CURRENT_RGB();
      
    // Create shadow / highlight colours at end of RGB array
    // The resultant values are the same as MAME
    r = r * 202 / 256;
    g = g * 202 / 256;
    b = b * 202 / 256;
        
    rgb[rgbAddr + Video::S16_PALETTE_ENTRIES] =
    rgb[rgbAddr + (Video::S16_PALETTE_ENTRIES * 2)] = CURRENT_RGB();
}
