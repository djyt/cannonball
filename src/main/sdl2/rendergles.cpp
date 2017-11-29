/***************************************************************************
    Open GL_ES Video Rendering.

    Copyright Manuel Alfayate.
    See license.txt for more details.
***************************************************************************/

#include <iostream>
#include <assert.h>

#include "rendergles.hpp"
#include "frontend/config.hpp"

// Uncomment only for debugging purposes.
void gles_show_error();
#define	SHOW_ERROR //gles_show_error();

const unsigned kVertexCount = 4;
const unsigned kIndexCount = 6;

static const char *vertex_shader =
   "attribute vec2 TexCoord;\n"
   "attribute vec2 VertexCoord;\n"
   "attribute vec4 Color;\n"
   "uniform mat4 MVPMatrix;\n"
   "varying vec2 tex_coord;\n"
   "void main() {\n"
   "   gl_Position = MVPMatrix * vec4(VertexCoord, 0.0, 1.0);\n"
   "   tex_coord = TexCoord;\n"
   "}";

static const char *fragment_shader =
   "#ifdef GL_ES\n"
   "precision mediump float;\n"
   "#endif\n"
   "uniform sampler2D Texture;\n"
   "varying vec2 tex_coord;\n"
   "void main() {\n"
   "   gl_FragColor = vec4(texture2D(Texture, tex_coord).rgb, 1.0);\n"
   "}";

const char* vertex_shader_scanlines =
"attribute vec4 VertexCoord;\n"
"attribute vec4 COLOR;\n"
"attribute vec4 TexCoord;\n"
"varying vec4 COL0;\n"
"varying vec4 TEX0;\n"
"varying vec2 omega;\n"

"vec4 _oPosition1;\n"
"uniform mat4 MVPMatrix;\n"
"uniform mediump int FrameDirection;\n"
"uniform mediump int FrameCount;\n"
"uniform mediump vec2 OutputSize;\n"
"uniform mediump vec2 TextureSize;\n"
"uniform mediump vec2 InputSize;\n"

"void main()\n"
"{\n"
"    gl_Position = MVPMatrix * VertexCoord;\n"
"    COL0 = COLOR;\n"
"    TEX0.xy = TexCoord.xy;\n"
"	omega = vec2(3.141592654 * OutputSize.x, 2.0 * 3.141592654 * TextureSize.y);\n"
"}"
;

const char* fragment_shader_scanlines =

"precision mediump float;\n"

"uniform mediump int FrameDirection;\n"
"uniform mediump int FrameCount;\n"
"uniform mediump vec2 OutputSize;\n"
"uniform mediump vec2 TextureSize;\n"
"uniform mediump vec2 InputSize;\n"
"uniform sampler2D Texture;\n"
"uniform mediump float SCANLINE_BASE_BRIGHTNESS;\n"

"varying vec4 TEX0;\n"
"varying vec2 omega;\n"

"mediump float SCANLINE_SINE_COMP_A = 0.0;\n"
"mediump float SCANLINE_SINE_COMP_B = 0.10;\n"

"void main()\n"
"{\n"
"   vec2 sine_comp = vec2(SCANLINE_SINE_COMP_A, SCANLINE_SINE_COMP_B);\n"
"   vec3 res = texture2D(Texture, TEX0.xy).xyz;\n"
"   vec3 scanline = res * (SCANLINE_BASE_BRIGHTNESS + dot(sine_comp * sin(TEX0.xy * omega), vec2(1.0, 1.0)));\n"
"   gl_FragColor = vec4(scanline.x, scanline.y, scanline.z, 1.0);\n"
"}"
; 

const GLfloat vertices[] =
{
	-0.5f, -0.5f, 0.0f,
	+0.5f, -0.5f, 0.0f,
	+0.5f, +0.5f, 0.0f,
	-0.5f, +0.5f, 0.0f,
};

//Values defined in gles2_create()
GLfloat uvs[8];

const GLushort indices[] =
{
	0, 1, 2,
	0, 2, 3,
};

RenderGLES::RenderGLES()
{
}

RenderGLES::~RenderGLES()
{
}

void RenderGLES::disable()
{
    glDeleteProgram(shader.program);
    glDeleteBuffers(3, buffers); SHOW_ERROR
    glDeleteTextures(1, &texture); SHOW_ERROR

    // Deinit SDL2 EGL context
    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(glcontext);   
}

bool RenderGLES::init(int src_width, int src_height,
                    int scale,
                    int video_mode,
                    int scanlines)
{
    const int bpp = 32;

    // We init the SDL2 EGL context here
    SDL_ShowCursor(SDL_DISABLE);
    window = SDL_CreateWindow(
        "Cannonball", 0, 0, 0, 0, 
        SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN_DESKTOP);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    glcontext = SDL_GL_CreateContext(window);

    this->src_width  = src_width;
    this->src_height = src_height;
    this->video_mode = video_mode;
    this->scanlines  = scanlines;

    // Frees (Deletes) existing surface
    if (surface)
        SDL_FreeSurface(surface);

    surface = SDL_CreateRGBSurface(0, 0, 0, 32, 0, 0, 0, 0);
    
    if (!surface) {
	    std::cerr << "Can't create rendering memory surface: " << SDL_GetError() << std::endl;
	    return false;
    }

    // We get the screen dimensions from the egl information member of the context object.
    // Final values may vary if aspect ratio correction is applied.
    SDL_DisplayMode current_videomode;
    SDL_GetCurrentDisplayMode(0, &current_videomode); 
    scn_width = current_videomode.w;
    scn_height = current_videomode.h;
    
    int orig_scn_mode_width = scn_width;
    int scn_xpos = 0;

    // We only consider fullscreenmodes, since that's what GL_ES 
    // is meant to be displayed on. So it's streching or not: nothing else.
    if (! (video_mode == video_settings_t::MODE_STRETCH)) {
	float ratio_width = float (src_width) / float (src_height);
        scn_width = scn_height * ratio_width;
        scn_xpos = (orig_scn_mode_width - scn_width) / 2; 
    }
   
    // The src and scn dimensions are only needed for the scanlines shaders
    gles2_init_shaders(src_width, src_height, scn_width, scn_height, scanlines);

    if (screen_pixels)
        delete[] screen_pixels;
    screen_pixels = new uint32_t[src_width * src_height];

    // SDL Pixel Format Information
    Rshift = surface->format->Rshift;
    Gshift = surface->format->Gshift;
    Bshift = surface->format->Bshift;

    // --------------------------------------------------------------------------------------------
    // Initalize Open GL
    // --------------------------------------------------------------------------------------------

    glDisable(GL_DITHER);
    glDisable(GL_DEPTH_TEST);

    glClearColor(0, 0, 0, 0); // Black background

    glViewport(scn_xpos, 0, scn_width, scn_height); 

    // Initalize Texture ID
    glGenTextures(1, &texture);

    // ---------- Screen texture setup  ------------------
    const GLint param = config.video.filtering ? GL_LINEAR : GL_NEAREST;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, param);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT,
		src_width, src_height, 0,
		GL_BGRA_EXT, GL_UNSIGNED_BYTE,
		NULL); SHOW_ERROR
  
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear screen and depth buffer
    
    glActiveTexture(GL_TEXTURE0); SHOW_ERROR
    
    // Leave screen texture binded so it's like that when we arrive to draw_frame so we save this call.
    glBindTexture(GL_TEXTURE_2D, texture); SHOW_ERROR

    // ---------- GL geometry setup  ------------------

    GLfloat uvs[8];   
    GLfloat proj[4][4];

    // Setup texture coordinates
    float min_u=0;
    float max_u=1.0f;
    float min_v=0;
    float max_v=1.0f;

    uvs[0] = min_u;
    uvs[1] = min_v;
    uvs[2] = max_u;
    uvs[3] = min_v;
    uvs[4] = max_u;
    uvs[5] = max_v;
    uvs[6] = min_u;
    uvs[7] = max_v;
    //

    // Init buffer data
    glGenBuffers(3, buffers); SHOW_ERROR
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]); SHOW_ERROR
    glBufferData(GL_ARRAY_BUFFER, kVertexCount * sizeof(GLfloat) * 3, vertices, GL_STATIC_DRAW); SHOW_ERROR
    glBindBuffer(GL_ARRAY_BUFFER, buffers[1]); SHOW_ERROR
    glBufferData(GL_ARRAY_BUFFER, kVertexCount * sizeof(GLfloat) * 2, uvs, GL_STATIC_DRAW); SHOW_ERROR
    glBindBuffer(GL_ARRAY_BUFFER, 0); SHOW_ERROR
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[2]); SHOW_ERROR
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, kIndexCount * sizeof(GL_UNSIGNED_SHORT), indices, GL_STATIC_DRAW); SHOW_ERROR
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); SHOW_ERROR
    //

    // Activate the vertex position and texture coordinate attributes for the vertices
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]); SHOW_ERROR
    glVertexAttribPointer(shader.a_position, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), NULL); SHOW_ERROR
    glEnableVertexAttribArray(shader.a_position); SHOW_ERROR

    glBindBuffer(GL_ARRAY_BUFFER, buffers[1]); SHOW_ERROR
    glVertexAttribPointer(shader.a_texcoord, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL); SHOW_ERROR
    glEnableVertexAttribArray(shader.a_texcoord); SHOW_ERROR

    // Set the projection matrix
    SetOrtho(proj, -0.5f, +0.5f, +0.5f, -0.5f, -1.0f, 1.0f, 1, 1);
    
    // Upload the projection matrix to the shader
    glUniformMatrix4fv(shader.u_vp_matrix, 1, GL_FALSE, &proj[0][0]); SHOW_ERROR
   
    // We leave the element array buffer binded so it's binded when we arrive to gles2_draw() on each frame.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[2]); SHOW_ERROR
 
    return true;
}

void gles_show_error()
{
	GLenum error = GL_NO_ERROR;
	error = glGetError();
	if (GL_NO_ERROR != error) {
		SDL_Log("GL Error %x encountered!\n", error);
	}
}

void RenderGLES::gles2_init_shaders (unsigned input_width, unsigned input_height,
	unsigned display_width, unsigned display_height, int scanlines) {
	
	memset(&shader, 0, sizeof(__ShaderInfo));

	// Load custom shaders
   	float input_size[2], output_size[2], texture_size[2];
	float scanline_bright;
	
	if (scanlines)
		shader.program = CreateProgram(vertex_shader_scanlines, fragment_shader_scanlines);
	else
		shader.program = CreateProgram(vertex_shader, fragment_shader);

	if(shader.program)
	{
		shader.u_vp_matrix   = glGetUniformLocation(shader.program, "MVPMatrix");
  	 	shader.a_texcoord    = glGetAttribLocation(shader.program, "TexCoord");
		shader.a_position    = glGetAttribLocation(shader.program, "VertexCoord");
		
		if (scanlines) {
			/* We need the texture height to be an exact divisor of the phisical videomode height
			 * to avoid patterns on the scanlines. */
			unsigned texture_width = input_width;
			unsigned texture_height = display_height / (display_height / input_height);

			shader.input_size      = glGetUniformLocation(shader.program, "InputSize");
			shader.output_size     = glGetUniformLocation(shader.program, "OutputSize");
			shader.texture_size    = glGetUniformLocation(shader.program, "TextureSize");
			shader.scanline_bright = glGetUniformLocation(shader.program, "SCANLINE_BASE_BRIGHTNESS"); 

			input_size  [0] = input_width;
			input_size  [1] = input_height;
			output_size [0] = display_width;
			output_size [1] = display_height;
			texture_size[0] = texture_width;
			texture_size[1] = texture_height;

			scanline_bright = 0.85;
		}
	}
	else	
		exit(0);

	glUseProgram(shader.program); SHOW_ERROR

	if (scanlines) {
		glUniform2fv(shader.input_size, 1, input_size);
		glUniform2fv(shader.output_size, 1, output_size);
		glUniform2fv(shader.texture_size, 1, texture_size);
		glUniform1f(shader.scanline_bright, scanline_bright);
	}
}

GLuint RenderGLES::CreateShader(GLenum type, const char *shader_src)
{
	GLuint shader = glCreateShader(type);
	if(!shader)
		return 0;

	// Load and compile the shader source
	glShaderSource(shader, 1, &shader_src, NULL);
	glCompileShader(shader);

	// Check the compile status
	GLint compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(!compiled)
	{
		GLint info_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
		if(info_len > 1)
		{
			char* info_log = (char *)malloc(sizeof(char) * info_len);
			glGetShaderInfoLog(shader, info_len, NULL, info_log);
			SDL_Log("Error compiling shader:\n%s\n", info_log);
			free(info_log);
		}
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

// Function to load both vertex and fragment shaders, and create the program
GLuint RenderGLES::CreateProgram(const char *vertex_shader_src, const char *fragment_shader_src)
{
	GLuint vertex_shader = CreateShader(GL_VERTEX_SHADER, vertex_shader_src);
	if(!vertex_shader)
		return 0;
	GLuint fragment_shader = CreateShader(GL_FRAGMENT_SHADER, fragment_shader_src);
	if(!fragment_shader)
	{
		glDeleteShader(vertex_shader);
		return 0;
	}

	GLuint program_object = glCreateProgram();
	if(!program_object)
		return 0;
	glAttachShader(program_object, vertex_shader);
	glAttachShader(program_object, fragment_shader);

	// Link the program
	glLinkProgram(program_object);

	// Check the link status
	GLint linked = 0;
	glGetProgramiv(program_object, GL_LINK_STATUS, &linked);
	if(!linked)
	{
		GLint info_len = 0;
		glGetProgramiv(program_object, GL_INFO_LOG_LENGTH, &info_len);
		if(info_len > 1)
		{
			char* info_log = (char *)malloc(info_len);
			glGetProgramInfoLog(program_object, info_len, NULL, info_log);
			printf("Error linking program:\n%s\n", info_log);
			free(info_log);
		}
		glDeleteProgram(program_object);
		return 0;
	}
	// Delete these here because they are attached to the program object.
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return program_object;
}

// Builds an orthographic projection matrix and stores it in the matrix on the first parameter.
void RenderGLES::SetOrtho(float m[4][4], float left, float right, float bottom, float top, float near, float far, float scale_x, float scale_y)
{
	memset(m, 0, 4*4*sizeof(float));
	m[0][0] = 2.0f/(right - left)*scale_x;
	m[1][1] = 2.0f/(top - bottom)*scale_y;
	m[2][2] = -2.0f/(far - near);
	m[3][0] = -(right + left)/(right - left);
	m[3][1] = -(top + bottom)/(top - bottom);
	m[3][2] = -(far + near)/(far - near);
	m[3][3] = 1;
}

bool RenderGLES::start_frame()
{
    return true;
}

bool RenderGLES::finalize_frame()
{
    return true;
}

void RenderGLES::draw_frame(uint16_t* pixels)
{
    uint32_t* spix = screen_pixels;

    // Lookup real RGB value from rgb array for backbuffer
    for (int i = 0; i < (src_width * src_height); i++)
        *(spix++) = rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,	       // target, LOD, xoff, yoff
            src_width, src_height,                     // texture width, texture height
            GL_BGRA_EXT,                               // format of pixel data
            GL_UNSIGNED_BYTE,               	       // data type of pixel data
            screen_pixels);                            // pointer in image memory
    
    glDrawElements(GL_TRIANGLES, kIndexCount, GL_UNSIGNED_SHORT, 0); SHOW_ERROR
   
    // We pageflip the SDL2 EGL context here
    SDL_GL_SwapWindow(window);
}
