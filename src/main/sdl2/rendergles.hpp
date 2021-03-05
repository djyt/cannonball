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

#include <SDL_opengles2.h>
#include "renderbase.hpp"

struct __ShaderInfo
{
   GLuint program;
   GLint u_vp_matrix;
   GLint u_texture;
   GLint a_position; // vertex_coord;
   GLint a_texcoord; //	tex_coord;
   GLint a_color;    // color
   
   GLint lut_tex_coord;

   GLint input_size;
   GLint output_size;
   GLint texture_size;
   GLfloat scanline_bright;
};

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
    bool supports_window() { return false; }
	bool supports_vsync();


private:
    // Texture IDs
    const static int SCREEN = 0;

    GLuint buffers[3];
    GLuint texture;

    struct __ShaderInfo shader; 
   
    void gles2_init_shaders (unsigned texture_width, unsigned texture_height, 
	unsigned output_width, unsigned output_height, int scanlines);

    GLuint CreateProgram(const char *vertex_shader_src, const char *fragment_shader_src);
    GLuint CreateShader(GLenum type, const char *shader_src);
    void SetOrtho(float m[4][4], float left, float right, float bottom, float top, float near, float far, float scale_x, float scale_y);

   SDL_GLContext glcontext;
   SDL_Window *window;
};
