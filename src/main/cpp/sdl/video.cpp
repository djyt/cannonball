#include "sdl/video.hpp"
    
Video video;

Video::Video(void)
{
    sprite_layer = new hwsprites();
    tile_layer = new hwtiles();
}

Video::~Video(void)
{
    delete sprite_layer;
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

    /*
    * Set our width/height to 640/480 (you would
    * of course let the user decide this in a normal
    * app). We get the bpp we will request from
    * the display. On X11, VidMode can't change
    * resolution, so this is probably being overly
    * safe. Under Win32, ChangeDisplaySettings
    * can change the bpp.
    */
    int width = S16_WIDTH;
    int height = S16_HEIGHT;
    //int bpp = info->vfmt->BitsPerPixel;
    int bpp = 32;
    int flags = SDL_DOUBLEBUF | SDL_SWSURFACE;
    
    int available = SDL_VideoModeOK(width, height, bpp, flags);

    // Set the video mode
    surface = SDL_SetVideoMode(width, height, bpp, flags);

    if (!surface || !available)
    {
        std::cerr << "Video mode set failed: " << SDL_GetError() << std::endl;
        return 0;
    }

    // Use this function to perform a fast, low quality, stretch blit between two surfaces of the same pixel format.
    // Investigate: http://wiki.libsdl.org/moin.cgi/SDL_SoftStretch

    // Convert the SDL pixel surface to 32 bit
    pixels = (uint32_t*)surface->pixels;

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

    // Iterate pxls backbuffer
    for (int i = 0; i < (surface->w * surface->h) ; i++) 
    {
        // Lookup real RGB value from rgb array for backbuffer
        pixels[i] = rgb[pixels[i] & ((S16_PALETTE_ENTRIES * 3) - 1)];
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
    //std::cout << std::hex << "pal adr: " << *palAddr << " data: " << data << std::endl;

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

void Video::refresh_palette(uint32_t palAddr)
{
    //See: SDL_PixelFormat
    #define CURRENT_RGB() (r << surface->format->Rshift) | (g << surface->format->Gshift) | (b << surface->format->Bshift);

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
