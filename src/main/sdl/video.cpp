#include "sdl/video.hpp"
    
Video video;

Video::Video(void)
{
    // Internal pixel array. The size of this is always a constant
    pixels = new uint32_t[S16_WIDTH * S16_HEIGHT];
    sprite_layer = new hwsprites();
    tile_layer = new hwtiles();
}

Video::~Video(void)
{
    delete sprite_layer;
    delete tile_layer;
    delete[] pixels;
}

int Video::init(uint8_t* tile_rom, uint8_t* sprite_rom, uint8_t* road_rom)
{
    // Information about the current video settings.
    const SDL_VideoInfo* info = SDL_GetVideoInfo();

    if (!info)
    {
        // This should probably never happen.
        std::cerr << "Video query failed: " << SDL_GetError() << std::endl;
        return 0;
    }

    if (!set_video_mode(MODE_NOSCALE))
        return 0;

    // Convert S16 tiles to a more useable format & del memory used by original
    tile_layer->init((uint8_t*) tile_rom);
    clear_tile_ram();
    clear_text_ram();
    delete[] tile_rom;

    // Convert S16 sprites
    sprite_layer->init((uint8_t*) sprite_rom);
    delete[] sprite_rom;

    // Convert S16 Road Stuff
    hwroad.init((uint8_t*) road_rom);
    delete[] road_rom;

    return 1;
}

// ---------------------------------------------------------------------------
// Choose Video Mode 
// ---------------------------------------------------------------------------

const static uint16_t VIDEO_MODES[] =
{
    //1680, 1050, 
    640, 480,                                        // Full Screen Mode (Must be bordered 640x480)
    S16_WIDTH * 1, S16_HEIGHT * 1,                   // 1:1 Original Size
    S16_WIDTH * 2, S16_HEIGHT * 2,                   // 2:1 Double Size
    S16_WIDTH * 3, S16_HEIGHT * 3,                   // 3:1 Triple Size
};

int Video::set_video_mode(uint8_t m)
{
    video_mode = m;

    screen_width  = VIDEO_MODES[(video_mode << 1) + 0];
    screen_height = VIDEO_MODES[(video_mode << 1) + 1];

    // Record how much we are scaling the screen by, from it's original resolution
    scale_factor = std::min(screen_width / S16_WIDTH, screen_height / S16_HEIGHT);

    screen_xoff = screen_width - (S16_WIDTH * scale_factor);
    if (screen_xoff)
        screen_xoff = (screen_xoff / 2);

    screen_yoff = screen_height - (S16_HEIGHT * scale_factor);
    if (screen_yoff) 
        screen_yoff = (screen_yoff / 2) * screen_width;
   
    //int bpp = info->vfmt->BitsPerPixel;
    int bpp = 32;
    int flags = SDL_DOUBLEBUF | SDL_SWSURFACE;

    if (video_mode == MODE_FULLSCREEN)
    {
        flags |= SDL_FULLSCREEN;
        SDL_ShowCursor(false); // Don't show mouse cursor in full-screen mode
    }
    else
        SDL_ShowCursor(true);
    
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

    return 1;
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
* Note that both srcwid&srchgt and dstwid&dsthgt refer to the source image's dimensions. 
* The destination page size is specified in pagewid&pagehgt.
* 
* @author Chris White
* 
*/
void Video::scale( uint32_t* src, int srcwid, int srchgt, 
                   uint32_t* dest, int dstwid, int dsthgt)
{
    int xstep = (srcwid << 16) / dstwid; // calculate distance (in source) between
    int ystep = (srchgt << 16) / dsthgt; // pixels (in dest)
    int srcy = 0; // y-cordinate in source image
        
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



void Video::draw_frame(void)
{
    if (SDL_MUSTLOCK(surface) && SDL_LockSurface(surface) < 0) 
        return;

    // ------------------------------------------------------------------------
    // Draw
    // ------------------------------------------------------------------------

    tile_layer->update_tile_values();

    hwroad.render_background(pixels);
    sprite_layer->render(1);
    tile_layer->render_tile_layer(pixels, 1, 0);
    sprite_layer->render(2);
    tile_layer->render_tile_layer(pixels, 1, 1);
    tile_layer->render_tile_layer(pixels, 0, 0);
    sprite_layer->render(4);
    tile_layer->render_tile_layer(pixels, 0, 1);
    hwroad.render_foreground(pixels);
    tile_layer->render_text_layer(pixels, 0);
    sprite_layer->render(8);
    tile_layer->render_text_layer(pixels, 1);

    // Do Scaling
    if (video_mode != MODE_NOSCALE)
    {
        // Lookup real RGB value from rgb array for backbuffer
        for (int i = 0; i < (S16_WIDTH * S16_HEIGHT); i++)    
            pixels[i] = rgb[pixels[i] & ((S16_PALETTE_ENTRIES * 3) - 1)];

        // Rescale appropriately
        scale(pixels, S16_WIDTH, S16_HEIGHT, 
              screen_pixels + screen_xoff + screen_yoff, S16_WIDTH * scale_factor, S16_HEIGHT * scale_factor);
    }
    // No Scaling
    else
    {
        // Lookup real RGB value from rgb array for backbuffer
        for (int i = 0; i < (S16_WIDTH * S16_HEIGHT); i++)    
            screen_pixels[i] = rgb[pixels[i] & ((S16_PALETTE_ENTRIES * 3) - 1)];
    }

    // Example: Set the pixel at 10,10 to red
    //pixels[( 10 * surface->w ) + 10] = 0xFF0000;
    // ------------------------------------------------------------------------

    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);

    SDL_Flip(surface);
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
#define CURRENT_RGB() (r << surface->format->Rshift) | (g << surface->format->Gshift) | (b << surface->format->Bshift);

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
        
    rgb[rgbAddr + Video::S16_PALETTE_ENTRIES] = CURRENT_RGB();
    rgb[rgbAddr + (Video::S16_PALETTE_ENTRIES * 2)] = CURRENT_RGB();
}