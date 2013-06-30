/***************************************************************************
    Open GL Video Rendering.  
    
    Useful References:
    http://www.sdltutorials.com/sdl-opengl-tutorial-basics
    http://www.opengl.org/wiki/Common_Mistakes
    http://open.gl/textures

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "renderbase.hpp"

//#define GL_GLEXT_PROTOTYPES 
#include <SDL_opengl.h>

class RenderGL : public RenderBase
{
public:
    RenderGL();
    bool init(int src_width, int src_height, 
              int scale,
              int video_mode,
              int scanlines);
    void disable();
    bool start_frame();
    bool finalize_frame();
    void draw_frame(uint32_t* pixels);

private:
    // Texture IDs
    const static int SCREEN = 0;
    const static int SCANLN = 1;

    GLuint textures[2];
    GLuint dlist; // GL display list
};