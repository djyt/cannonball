/*******************************************************************************
    SDL2 Hardware Surface Video Rendering.

    Copyright (c) 2012,2020 Manuel Alfayate and Chris White.
    Threading, Blargg integration and CRT masks Copyright (c) 2020 James Pearce

    See license.txt for more details.

*******************************************************************************/

#pragma once

#include "renderbase.hpp"
#include "snes_ntsc.h"

class RenderSurface : public RenderBase
{
public:
    RenderSurface();
    ~RenderSurface();
    bool init(int src_width, int src_height,
              int scale,
              int video_mode,
              int scanlines);
    void disable();
    bool start_frame();
//    void colorise_frame(int threadid, int TotalThreads,
//                        uint32_t *gameimage, uint32_t *coloredimage, uint32_t *intensityimage,
//                        int width, int height);
    bool finalize_frame();
    void draw_frame(uint16_t* pixels);

private:
    // SDL2 window
    SDL_Window *window;

    // SDL2 renderer
    SDL_Renderer *renderer;

    // SDL2 texture
    SDL_Texture *game_tx;      // game image
    SDL_Texture *last_game_tx; // game image from last frame - used to create phosphor decay
    SDL_Texture *scanline_tx;  // scanline overlay
    SDL_Texture *rgb_tx1;      // vertical rgb texture overlay
    SDL_Texture *overlay;      // used to control image brightness
    SDL_Texture *vignette_tx;  // vignette overlay (used for edges)

    // SDL2 blitting rects for hw scaling
    // ratio correction using SDL_RenderCopy()
    SDL_Rect src_rect;
    SDL_Rect scanline_rect;
    SDL_Rect rgb_rect;
    SDL_Rect dst_rect;

    // internal functions
    void init_blargg_filter();
    bool init_sdl(int video_mode);
    void init_textures();
    void init_overlays();
    void create_color_curves();
    void colorise_frame( int ThreadID, int TotalThreads,
                        uint32_t *gameimage, uint32_t *coloredimage, uint32_t *intensityimage,
                        int width, int height);
    void shade(int ThreadID, int TotalThreads); //, uint16_t* pixels);

    // constants
    const int BPP = 32;

    // Blargg filter related
    snes_ntsc_setup_t setup;
    snes_ntsc_t* ntsc = 0;
    int snes_src_width;
    int phase;
    int phaseframe;

    // currently configured video settings. These are stored so that a change can be actioned.
    int scale;
    int flags;              // SDL flags
    int shader;             // current Blargg filter value
    int vignette;           // current vignette value
    int crtmask = 0;        // current mask
    int mask_intensity = 0; // current mask level
    int crtfade = 0;        // current crtfade level
    int overdrive;          // current overdrive
    int flicker;            // true is screen flicker should be added with vignette
    int red_gain    = 1;
    int green_gain  = 1;
    int blue_gain   = 1;
    int red_curve   = 1;
    int green_curve = 1;
    int blue_curve  = 1;

    // processing data
    uint32_t screen_r[256]; // rainbow table to lookup exponent values for red channel
    uint32_t screen_g[256]; // rainbow table to lookup exponent values for green channel
    uint32_t screen_b[256]; // rainbow table to lookup exponent values for blue channel
    int Alevel = 192;       // default alpha value for game image

    // working buffers for video processing
    uint32_t *game_pixels;
    uint32_t *overlay_pixels;
      // Blargg filtering buffers
    uint16_t *rgb_pixels = 0;            // used by Blargg filter
    uint16_t *filtered_rgb_pixels = 0;   // used by Blargg filter for RGB output
    uint32_t *filtered_pixels = 0;       // used by Blargg filter for native output
};
