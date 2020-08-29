/***************************************************************************
    SDL2 Hardware Surface Video Rendering.  
    
    Known Bugs:
    - Software scanlines not implemented because we do hardware post-scaling
      using the SDL_RenderCopy() rects from the original bitmap, so would not
      look good at all because they would be ruined by magnifying.

    Copyright Manuel Alfayate and Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "renderbase.hpp"

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
    void colorise_frame(int threadid, int TotalThreads,
                        uint32_t *gameimage, uint32_t *coloredimage, uint32_t *intensityimage,
                        int width, int height);
    bool finalize_frame();
    void shade(int ThreadID, int TotalThreads); //, uint16_t* pixels);
    void draw_frame(uint16_t* pixels);

private:
    // SDL2 window
    SDL_Window *window;

    // SDL2 renderer
    SDL_Renderer *renderer;

    // SDL2 texture
    SDL_Texture *underlay;     // used to control black level
    SDL_Texture *game_tx;      // game image
    SDL_Texture *last_game_tx; // game image from last frame - used to create phosphor decay
    SDL_Texture *game_tx2;     // game image
    SDL_Texture *scanline_tx;  // scanline overlay
    SDL_Texture *rgb_tx1;      // vertical rgb texture overlay - phase 1 (interlacing)
    SDL_Texture *rgb_tx2;      // vertical rgb texture overlay - phase 2 (interlacing)
    SDL_Texture *overlay;      // used to control image brightness
    SDL_Texture *vignette_tx;  // vignette overlay (used for edges)
    SDL_Texture *reflection_tx;// white splodge overlay, simulates general reflection on glass

    // SDL2 blitting rects for hw scaling
    // ratio correction using SDL_RenderCopy()
    SDL_Rect src_rect;
    SDL_Rect scanline_rect;
    SDL_Rect rgb_rect;
    SDL_Rect dst_rect;
};
