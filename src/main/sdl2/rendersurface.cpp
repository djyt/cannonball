/***************************************************************************
    SDL2 Hardware Surface Video Rendering.  
    
    Known Bugs:
    - Scanlines don't work when Endian changed?

    Copyright Manuel Alfayate, Chris White.
    See license.txt for more details.
***************************************************************************/

#include <iostream>

#include "rendersurface.hpp"
#include "frontend/config.hpp"

RenderSurface::RenderSurface()
{
}

RenderSurface::~RenderSurface()
{
}

bool RenderSurface::init(int src_width, int src_height, 
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

    // In SDL2, we calculate the output dimensions, but then in draw_frame() we won't do any scaling: SDL2
    // will do that for us, using the rects passed to SDL_RenderCopy().
    // scn_* -> physical screen dimensions OR window dimensions. On FULLSCREEN MODE it has the physical screen
    //		dimensions and in windowed mode it has the window dimensions.
    // src_* -> real, internal, frame dimensions. Will ALWAYS be 320 or 398 x 224. NEVER CHANGES. 
    // corrected_scn_width_* -> output screen size for scaling.
    // In windowed mode it's the size of the window. 
   
    // --------------------------------------------------------------------------------------------
    // Full Screen Mode
    // --------------------------------------------------------------------------------------------
    if (video_mode == video_settings_t::MODE_FULL || video_mode == video_settings_t::MODE_STRETCH)
    {
	flags |= (SDL_WINDOW_FULLSCREEN); // Set SDL flag

	// Fullscreen window size: SDL2 ignores w and h in SDL_CreateWindow() if FULLSCREEN flag
	// is enable, which is fine, so the window will be fullscreen of the physical videomode
	// size, but then, if we want to preserve ratio, we need dst_width bigger than src_width.	
	scn_width  = orig_width;
        scn_height = orig_height;

	src_rect.w = src_width;
	src_rect.h = src_height;
	src_rect.x = 0;
	src_rect.y = 0;
	dst_rect.y = 0;

	float x_ratio = float(src_width) / float(src_height);
	int corrected_scn_width = scn_height * x_ratio; 

	if (!(video_mode == video_settings_t::MODE_STRETCH)) {
		float x_ratio = float(src_width) / float(src_height);
		int corrected_scn_width = scn_height * x_ratio; 
		dst_rect.w = corrected_scn_width;
		dst_rect.h = scn_height;
		dst_rect.x = (scn_width - corrected_scn_width) / 2;
	}
	else {
		dst_rect.w = scn_width;
		dst_rect.h = scn_height;
		dst_rect.x = 0;
	}

        SDL_ShowCursor(false);
     }
   
    // --------------------------------------------------------------------------------------------
    // Windowed Mode
    // --------------------------------------------------------------------------------------------
    else
    {
        this->video_mode = video_settings_t::MODE_WINDOW;
       
        scn_width  = src_width  * scale;
        scn_height = src_height * scale;

	src_rect.w = src_width;
	src_rect.h = src_height;
	src_rect.x = 0;
	src_rect.y = 0;
	dst_rect.w = scn_width;
	dst_rect.h = scn_height;
	dst_rect.x = 0;
	dst_rect.y = 0;

        SDL_ShowCursor(true);
    }

    //int bpp = info->vfmt->BitsPerPixel;
    const int bpp = 32;

    // Frees (Deletes) existing surface
    if (surface)
        SDL_FreeSurface(surface);

    surface = SDL_CreateRGBSurface(0,
                                  src_width,
                                  src_height,
                                  bpp,
                                  0,
                                  0,
                                  0,
                                  0);

    if (!surface)
    {
        std::cerr << "Surface creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear"); 
    window = SDL_CreateWindow(
        "Cannonball", 0, 0, scn_width, scn_height, 
        flags);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    texture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               src_width, src_height);

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

    return true;
}

void RenderSurface::disable()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

bool RenderSurface::start_frame()
{
    return true;
}

bool RenderSurface::finalize_frame()
{
    // SDL2 block
    SDL_UpdateTexture(texture, NULL, screen_pixels, src_width * sizeof (Uint32));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, &src_rect, &dst_rect);
    SDL_RenderPresent(renderer);
    //

    return true;
}

void RenderSurface::draw_frame(uint16_t* pixels)
{
    uint32_t* spix = screen_pixels;

    // Lookup real RGB value from rgb array for backbuffer
    for (int i = 0; i < (src_width * src_height); i++)
	*(spix++) = rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
}
