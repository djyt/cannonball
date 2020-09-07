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

class Render : public RenderBase
{
public:
    Render();
    ~Render();
    bool init(int src_width, int src_height, 
              int scale,
              int video_mode,
              int scanlines);
    void disable();
    bool start_frame();
    bool finalize_frame();
    void draw_frame(uint16_t* pixels);

private:
    // SDL2 window
    SDL_Window *window;

    // SDL2 renderer
    SDL_Renderer *renderer;

    // SDL2 texture
    SDL_Texture *texture;

    // SDL2 blitting rects for hw scaling 
    // ratio correction using SDL_RenderCopy()
    SDL_Rect src_rect;
    SDL_Rect dst_rect;
};
