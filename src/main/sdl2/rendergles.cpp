/***************************************************************************
    Open GL_ES Video Rendering.

    Copyright Manuel Alfayate.
    See license.txt for more details.
***************************************************************************/

#include <iostream>
#include <cstring>
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
	"uniform mat4 MVPMatrix;"
	"uniform mediump vec2 OutputSize;"
	"uniform mediump vec2 TextureSize;"
	"uniform mediump vec2 InputSize;"

	"attribute vec4 VertexCoord;"
	"attribute vec4 TexCoord;"
	"varying vec4 TEX0;"
	"varying vec4 TEX2;"
	"varying     vec2 _omega;"

	"struct sine_coord {"
	"    vec2 _omega;"
	"};"

	"vec4 _oPosition1;"
	"vec4 _r0006;"
	 
	"void main()"
	"{"
	"    vec2 _oTex;"
	"    sine_coord _coords;"
	"    _r0006 = VertexCoord.x*MVPMatrix[0];"
	"    _r0006 = _r0006 + VertexCoord.y*MVPMatrix[1];"
	"    _r0006 = _r0006 + VertexCoord.z*MVPMatrix[2];"
	"    _r0006 = _r0006 + VertexCoord.w*MVPMatrix[3];"
	"    _oPosition1 = _r0006;"
	"    _oTex = TexCoord.xy;"
	"    _coords._omega = vec2((3.14150000E+00*OutputSize.x*TextureSize.x)/InputSize.x, 6.28299999E+00*TextureSize.y);"
	"    gl_Position = _r0006;"
	"    TEX0.xy = TexCoord.xy;"
	"    TEX2.xy = _coords._omega;"
	"}";

const char* fragment_shader_scanlines =
	"precision mediump float;"

	"uniform mediump vec2 OutputSize;"
	"uniform mediump vec2 TextureSize;"
	"uniform mediump vec2 InputSize;"
	"uniform sampler2D Texture;"

	"varying vec2 _omega;"
	"varying vec4 TEX2;"
	"varying vec4 TEX0;"

	"struct sine_coord {"
	"    vec2 _omega;"
	"};"
	"vec4 _ret_0;"
	"float _TMP2;"
	"vec2 _TMP1;"
	"float _TMP4;"
	"float _TMP3;"
	"vec4 _TMP0;"
	"vec2 _x0009;"
	"vec2 _a0015;"

	"void main()"
	"{"
	"    vec3 _scanline;"
	"    _TMP0 = texture2D(Texture, TEX0.xy);"
	"    _x0009 = TEX0.xy*TEX2.xy;"
	"    _TMP3 = sin(_x0009.x);"
	"    _TMP4 = sin(_x0009.y);"
	"    _TMP1 = vec2(_TMP3, _TMP4);"
	"    _a0015 = vec2( 5.00000007E-02, 1.50000006E-01)*_TMP1;"
	"    _TMP2 = dot(_a0015, vec2( 1.00000000E+00, 1.00000000E+00));"
	"    _scanline = _TMP0.xyz*(9.49999988E-01 + _TMP2);"
	"    _ret_0 = vec4(_scanline.x, _scanline.y, _scanline.z, 1.00000000E+00);"
	"    gl_FragColor = _ret_0;"
	"    return;"
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

Render::Render()
{
}

Render::~Render()
{
}

void Render::disable()
{
    glDeleteProgram(shader.program);
    glDeleteBuffers(3, buffers); SHOW_ERROR
    glDeleteTextures(1, &texture); SHOW_ERROR

    // Deinit SDL2 EGL context
    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(glcontext);   
}

bool Render::init(int src_width, int src_height,
                    int scale,
                    int video_mode,
                    int scanlines)
{
	this->src_width = src_width;
	this->src_height = src_height;
	this->video_mode = video_mode;
	this->scanlines = scanlines;

	// Setup SDL Screen size
	if (!RenderBase::sdl_screen_size())
		return false;

	int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP;

    const int bpp = 32;

    // We init the SDL2 EGL context here
    SDL_ShowCursor(SDL_DISABLE);

	SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
	SDL_SetHint(SDL_HINT_VIDEO_WIN_D3DCOMPILER, "none");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	window = SDL_CreateWindow("Cannonball", 0, 0, 0, 0, flags);

    glcontext = SDL_GL_CreateContext(window);

    // Frees (Deletes) existing surface
    if (surface)
        SDL_FreeSurface(surface);

    surface = SDL_CreateRGBSurface(0, 0, 0, 32, 0, 0, 0, 0);
    
    if (!surface) {
	    std::cerr << "Can't create rendering memory surface: " << SDL_GetError() << std::endl;
	    return false;
    }


	// GL_ES only supports full screen mode
	// Calculate how much to scale screen from its original resolution
	
	// Full Screen Mode
	if (video_mode == video_settings_t::MODE_FULL)
	{
		// Calculate how much to scale screen from its original resolution
		uint32_t w = (scn_width << 16) / src_width;
		uint32_t h = (scn_height << 16) / src_height;
		dst_width = (src_width * std::min(w, h)) >> 16;
		dst_height = (src_height * std::min(w, h)) >> 16;
	}
	// Stretch screen. Lose original proportions
	else
	{
		dst_width = scn_width;
		dst_height = scn_height;
	}

	// If we're not stretching the screen, centre the image
	if (video_mode != video_settings_t::MODE_STRETCH)
	{
		screen_xoff = scn_width - dst_width;
		if (screen_xoff)
			screen_xoff = (screen_xoff / 2);

		screen_yoff = scn_height - dst_height;
		if (screen_yoff)
			screen_yoff = (screen_yoff / 2);
	}
	// Otherwise set to the top-left corner
	else
	{
		screen_xoff = 0;
		screen_yoff = 0;
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

    glDisable(GL_DITHER);		// Disable Dithering
    glDisable(GL_DEPTH_TEST);	// Disable Depth Buffer

    glClearColor(0, 0, 0, 0);	// Black background

    glViewport(screen_xoff, screen_yoff, dst_width, dst_height);

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

void Render::gles2_init_shaders (unsigned texture_width, unsigned texture_height,
	unsigned output_width, unsigned output_height, int scanlines) {
	
	memset(&shader, 0, sizeof(__ShaderInfo));

	// Load custom shaders
   	float input_size[2], output_size[2], texture_size[2];
	
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
			shader.input_size    = glGetUniformLocation(shader.program, "InputSize");
			shader.output_size   = glGetUniformLocation(shader.program, "OutputSize");
			shader.texture_size  = glGetUniformLocation(shader.program, "TextureSize");
			input_size [0]  = (float) texture_width;
			input_size [1]  = (float) texture_height;
			output_size[0]  = (float) output_width;
			output_size[1]  = (float) output_height;
			texture_size[0] = (float) texture_width;
			texture_size[1] = (float) texture_height;
		}
	}
	else	
		exit(0);

	glUseProgram(shader.program); SHOW_ERROR

	if (scanlines) 
	{
		glUniform2fv(shader.input_size, 1, input_size);
		glUniform2fv(shader.output_size, 1, output_size);
		glUniform2fv(shader.texture_size, 1, texture_size);
	}
}

GLuint Render::CreateShader(GLenum type, const char *shader_src)
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
GLuint Render::CreateProgram(const char *vertex_shader_src, const char *fragment_shader_src)
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
void Render::SetOrtho(float m[4][4], float left, float right, float bottom, float top, float near, float far, float scale_x, float scale_y)
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

bool Render::start_frame()
{
    return true;
}

bool Render::finalize_frame()
{
    return true;
}

void Render::draw_frame(uint16_t* pixels)
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
