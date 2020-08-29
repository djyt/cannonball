/***************************************************************************
    Open GL Video Rendering.

    Useful References:
    http://www.sdltutorials.com/sdl-opengl-tutorial-basics
    http://www.opengl.org/wiki/Common_Mistakes
    http://open.gl/textures

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <iostream>

#include "rendergl.hpp"
#include "frontend/config.hpp"

// Blargg filter for CRT processing (JJP) - with SMP support
#include "snes_ntsc.h"
#include <omp.h>

const static uint32_t SCANLINE_TEXTURE[] = { 0x00000000, 0xff000000 }; // BGRA 8-8-8-8-REV

// JJP - Blargg filtering for CRT style visuals
static snes_ntsc_setup_t setup;
static snes_ntsc_t* ntsc = 0;
static int shader;
static uint16_t *rgb_pixels = 0;            // used by Blarrg filter
static uint16_t *filtered_rgb_pixels = 0;   // used by Blarrg filter for RGB output
static uint32_t *filtered_pixels = 0;       // used by Blarrg filter for native output
static int phase;
static int phaseframe;
static int snes_src_width;
static int framesrendered;
static int framesdropped;
static uint32_t framenumber;
static double starttime;


RenderGL::RenderGL()
{

}

bool RenderGL::init(int src_width, int src_height,
                    int scale,
                    int video_mode,
                    int scanlines)
{
    this->src_width  = src_width;
    this->src_height = src_height;
    this->video_mode = video_mode;
    this->scanlines  = scanlines;

    // JJP - Blargg CRT filtering
    (config.video.blargg > video_settings_t::BLARGG_DISABLE) ? shader = 1 : shader = 0; // enable Blargg based on selected option
    framesrendered = 0;
    framesdropped = 0;
    starttime = omp_get_wtime();

    // JJP - add Blargg CRT filtering
    if (shader) {
//        printf("Shader running with %i threads.\n",config.video.blarggthreads);
        // first calculate the resultant image size.
        snes_src_width = SNES_NTSC_OUT_WIDTH( src_width );
        if (config.video.hires) snes_src_width = snes_src_width >> 1;
        snes_src_width++; // not sure why!
        if (ntsc)
            free(ntsc); // free memory
        ntsc = (snes_ntsc_t*) malloc( sizeof (snes_ntsc_t) );

        // configure selcted filtering type
        switch (config.video.blargg) {
            case video_settings_t::BLARGG_COMPOSITE:
                setup = snes_ntsc_composite;
                break;
            case video_settings_t::BLARGG_SVIDEO:
                setup = snes_ntsc_svideo;
                break;
            case video_settings_t::BLARGG_RGB:
                setup = snes_ntsc_rgb;
                break;
            case video_settings_t::BLARGG_MONO:
                setup = snes_ntsc_monochrome;
                break;
        }

        setup.merge_fields = 0;         // mimic interlacing
        setup.sharpness = -0.5;         // these values are based on observation only -
        setup.gamma     = -0.2;         //  nothing scientific!
        phase = 0;                      // initial frame will be phase 0
        phaseframe = 0;                 // used to track phase at 60 fps
        snes_ntsc_init( ntsc, &setup ); // configure the library
   }

    // Setup SDL Screen size
    if (!RenderBase::sdl_screen_size())
        return false;

    int flags = SDL_OPENGL;

    // Full Screen Mode
    if (video_mode == video_settings_t::MODE_FULL)
    {
        // Calculate how much to scale screen from its original resolution
        uint32_t w = (scn_width  << 16)  / src_width;
        uint32_t h = (scn_height << 16)  / src_height;
        dst_width  = (src_width  * std::min(w, h)) >> 16;
        dst_height = (src_height * std::min(w, h)) >> 16;
        flags |= SDL_FULLSCREEN; // Set SDL flag
        SDL_ShowCursor(false);   // Don't show mouse cursor in full-screen mode
    }
    // Stretch screen. Lose original proportions
    else if (video_mode == video_settings_t::MODE_STRETCH)
    {
        dst_width  = scn_width;
        dst_height = scn_height;
        flags |= SDL_FULLSCREEN; // Set SDL flag
        SDL_ShowCursor(false);   // Don't show mouse cursor in full-screen mode
    }
    // Window Mode
    else
    {
        scn_width  = dst_width  = src_width  * scale;
        scn_height = dst_height = src_height * scale;
        SDL_ShowCursor(true);
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

    //int bpp = info->vfmt->BitsPerPixel;
    const int bpp = 32;
    const int available = SDL_VideoModeOK(scn_width, scn_height, bpp, flags);

    // Frees (Deletes) existing surface
    if (surface)
        SDL_FreeSurface(surface);

    // Set the video mode
    surface = SDL_SetVideoMode(scn_width, scn_height, bpp, flags);

    if (!surface || !available)
    {
        std::cerr << "Video mode set failed: " << SDL_GetError() << std::endl;
        return false;
    }

    if (screen_pixels)
        delete[] screen_pixels;
    screen_pixels = new uint32_t[src_width * src_height];

    // JJP - Additional buffers for Blarrg filters
    if (rgb_pixels)
        delete[] rgb_pixels;
    rgb_pixels = new uint16_t[src_width * src_height];

    if (filtered_rgb_pixels)
        delete[] filtered_rgb_pixels;
    filtered_rgb_pixels = new uint16_t[(snes_src_width+1) * src_height];
    // note - we add 1 (word) to the width, as we need to do the same in the display routine

    if (filtered_pixels)
        delete[] filtered_pixels;
    filtered_pixels = new uint32_t[(snes_src_width+1) * src_height];
    // note - we add 1 (word) to the width, as we need to do the same in the display routine


    // SDL Pixel Format Information
    Rshift = surface->format->Rshift;
    Gshift = surface->format->Gshift;
    Bshift = surface->format->Bshift;

    // This hack is necessary to fix an Apple OpenGL with SDL issue
    #ifdef __APPLE__
      #if SDL_BYTEORDER == SDL_LIL_ENDIAN
        Rmask = 0x000000FF;
        Gmask = 0x0000FF00;
        Bmask = 0x00FF0000;
        Rshift += 8;
        Gshift -= 8;
        Bshift += 8;
      #else
        Rmask = 0xFF000000;
        Gmask = 0x00FF0000;
        Bmask = 0x0000FF00;
      #endif
    #else
        Rmask  = surface->format->Rmask;
        Gmask  = surface->format->Gmask;
        Bmask  = surface->format->Bmask;
    #endif

    // --------------------------------------------------------------------------------------------
    // Initalize Open GL
    // --------------------------------------------------------------------------------------------

    // Disable dithering
    glDisable(GL_DITHER);
    // Disable anti-aliasing
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POINT_SMOOTH);
    // Disable depth buffer
    glDisable(GL_DEPTH_TEST);

    // V-Sync
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

    glClearColor(0, 0, 0, 0); // Black background
    glShadeModel(GL_FLAT);

    glViewport(0, 0, scn_width, scn_height);

    // Initalize Texture ID
    glGenTextures(scanlines ? 2 : 1, textures);

    // Screen Texture Setup
    const GLint param = config.video.filtering ? GL_LINEAR : GL_NEAREST;
    glBindTexture(GL_TEXTURE_2D, textures[SCREEN]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, param);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                src_width, src_height, 0,                // texture width, texture height
                GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,    // Data format in pixel array
                NULL);

    // JJP - Blarrg shader texture setup
    if (shader) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                    snes_src_width, src_height, 0,            // texture width, texture height
//                    GL_RGB, GL_UNSIGNED_SHORT_5_6_5,          // Data format in pixel array
                    GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,    // Data format in pixel array
                    NULL);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                    src_width, src_height, 0,                // texture width, texture height
                    GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,    // Data format in pixel array
                    NULL);
    }


    // Scanline Texture Setup
    if (scanlines)
    {
        glBindTexture(GL_TEXTURE_2D, textures[SCANLN]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     1, 2, 0,
                     GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                     SCANLINE_TEXTURE);
    }

    // Initalize D-List
    dlist = glGenLists(1);
    glNewList(dlist, GL_COMPILE);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, scn_width, scn_height, 0, 0, 1);         // left, right, bottom, top, near, far
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear screen and depth buffer
    glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textures[SCREEN]);     // Select Screen Texture
        glBegin(GL_QUADS);
            glTexCoord2i(0, 1);
            glVertex2i  (screen_xoff,             screen_yoff + dst_height);  // lower left
            glTexCoord2i(0, 0);
            glVertex2i  (screen_xoff,             screen_yoff);               // upper left
            glTexCoord2i(1, 0);
            glVertex2i  (screen_xoff + dst_width, screen_yoff);               // upper right
            glTexCoord2i(1, 1);
            glVertex2i  (screen_xoff + dst_width, screen_yoff + dst_height);  // lower right
        glEnd();

        if (scanlines)
        {
            glEnable(GL_BLEND);
                glColor4ub(255, 255, 255, ((scanlines - 1) << 8) / 100);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glBindTexture(GL_TEXTURE_2D, textures[SCANLN]);
                glBegin(GL_QUADS);
                    glTexCoord2i(0, S16_HEIGHT);
                    glVertex2i  (screen_xoff,             screen_yoff + dst_height);  // lower left
                    glTexCoord2i(0, 0);
                    glVertex2i  (screen_xoff,             screen_yoff);               // upper left
                    glTexCoord2i(src_width, 0);
                    glVertex2i  (screen_xoff + dst_width, screen_yoff);               // upper right
                    glTexCoord2i(src_width, S16_HEIGHT);
                    glVertex2i  (screen_xoff + dst_width, screen_yoff + dst_height);  // lower right
                glEnd();
            glDisable(GL_BLEND);
        }
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
    glEndList();

    return true;
}

void RenderGL::disable()
{
    glDeleteLists(dlist, 1);
    glDeleteTextures(scanlines ? 2 : 1, textures);
}

bool RenderGL::start_frame()
{
    return !(SDL_MUSTLOCK(surface) && SDL_LockSurface(surface) < 0);
}

bool RenderGL::finalize_frame()
{
    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);

    return true;
}

void RenderGL::shade(int ThreadID, int TotalThreads, uint32_t* pixels)
{
    // JJP - Blarrg CRT filtering shading function itself.
    // As this is CPU intensive for e.g. RPi, this can be called in multiple threads
    // each processing a partial amount of the display.
    register uint32_t current_val;
    register uint16_t current_rgb_val;
    uint32_t* current_pixel = pixels;
    uint16_t* th_rgb_pixels = rgb_pixels;
    uint16_t* th_filtered_rgb_pixels;
    uint32_t* th_filtered_pixels;
    uint32_t start;
    uint32_t end;

    int block_rows = src_height / TotalThreads;
    int first_block_rows = block_rows;

    if (ThreadID==(TotalThreads-1)) {
         // add any remainder to the last block processed
        block_rows += (src_height % TotalThreads);
    }
    // calculate how much data to process in this thread
    start = src_width * first_block_rows * ThreadID;
    end   = start + (src_width * block_rows);
    current_pixel += start;
    th_rgb_pixels += start;

    if (framenumber==1) {
//        printf("src_width: %i, snes_src_width: %i, src_height: %i, threads: %i\n",
//              src_width, snes_src_width, src_height, TotalThreads);
//      printf("first_block_rows: %i, block_rows: %i, start: %i, end: %i\n",
//              first_block_rows, block_rows, start, end);
        printf("ThreadID: %i\n",ThreadID);
    }

    // convert pixel data to format used by Blarrg filtering code
    for (int i = start; i < end; i++) {
        current_val = rgb[*(current_pixel++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(th_rgb_pixels++) = uint16_t( ((uint16_t((current_val & Rmask) >> Rshift) & 0x00F8) << 8) +   // red
                                       ((uint16_t((current_val & Gmask) >> Gshift) & 0x00FC) << 3) +   // green
                                       ((uint16_t((current_val & Bmask) >> Bshift) & 0x00F8) >> 3) );  // blue
    }

    long output_pitch = (snes_src_width * 2); // 2 bytes-per-pixel (5/6/5), +2 (not sure why)
    th_rgb_pixels = rgb_pixels + start; // move back to start of data processed
    start = (output_pitch >> 1) * first_block_rows * ThreadID; // start position in filtered array
    end   = start + ((output_pitch >> 1) * block_rows);        // ..and the end position
    th_filtered_rgb_pixels = filtered_rgb_pixels + start;
    th_filtered_pixels = filtered_pixels + start;

    // Now call the blargg code, to do the work...
    if (config.video.hires) {
        snes_ntsc_blit_hires( ntsc, th_rgb_pixels, src_width, phase,
                              src_width, block_rows, th_filtered_rgb_pixels, output_pitch );
    } else {
        // standard res processing
        snes_ntsc_blit( ntsc, th_rgb_pixels, src_width, phase,
                        src_width, block_rows, th_filtered_rgb_pixels, output_pitch );
    }

    // finally, convert pixel data back to format preferred by OpenGL (it's faster to do this here)
    for (int i = start; i < end; i++) {
        current_rgb_val = *(th_filtered_rgb_pixels++);
        *(th_filtered_pixels++) = uint32_t( (uint32_t((current_rgb_val >> 8) & 0x00F8) << Rshift) +   // Red
                                            (uint32_t((current_rgb_val >> 3) & 0x00FC) << Gshift) +   // Green
                                            (uint32_t((current_rgb_val << 3) & 0x00F8) << Bshift) +   // Blue
                                            (uint32_t(0x0001)) );                                     // A
    }
}



void RenderGL::draw_frame(uint32_t* pixels)
// JJP - updated to provide optional Blarrg CRT filtering
{
    uint32_t* spix  = screen_pixels;
    uint16_t* sprgb = rgb_pixels;

    framesrendered++;
    if (shader) {
        // Blargg filtering provides more of a CRT look.
        framenumber++;
        uint32_t blarggstart = SDL_GetTicks(); // start timer
        // process this in specified number of threads
        int ID;
        if (config.video.blarggthreads>1) {
            omp_set_num_threads(config.video.blarggthreads);
            #pragma omp parallel
            {
                ID = omp_get_thread_num();
                shade(ID,config.video.blarggthreads,pixels);
            }
        } else {
            shade(0,1,pixels);
        }
        // update phase.  We need to consider whether we're running at 30 or 60 fps
        if (config.fps == 60) {
                if (++phaseframe == 2) {
                        phase ^= 1;
                        phaseframe = 0;
                }
        } else phase ^= 1; // 0 -> 1 -> 0 -> 1...
        *blarggtime += (SDL_GetTicks() - blarggstart);
        // display frame
        glBindTexture(GL_TEXTURE_2D, textures[SCREEN]);
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,            // target, LOD, xoff, yoff
            snes_src_width,                            // texture width (changed via Blargg filter)
            src_height,                                // texture height
            GL_BGRA,                                   // format of pixel data
            GL_UNSIGNED_INT_8_8_8_8_REV,               // data type of pixel data
            filtered_pixels);                          // pointer in image memory */
/*                  GL_RGB,                                    // format of pixel data
            GL_UNSIGNED_SHORT_5_6_5,                   // data type of pixel data
            filtered_rgb_pixels);                      // pointer in image memory  */
        glCallList(dlist);
        SDL_GL_SwapBuffers();
    } else {
        // Standard unfiltered display mode
        // Lookup real RGB value from rgb array for backbuffer
        for (int i = 0; i < (src_width * src_height); i++)
            *(spix++) = rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];

        glBindTexture(GL_TEXTURE_2D, textures[SCREEN]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,            // target, LOD, xoff, yoff
                src_width, src_height,                     // texture width, texture height
                GL_BGRA,                                   // format of pixel data
                GL_UNSIGNED_INT_8_8_8_8_REV,               // data type of pixel data
                screen_pixels);                            // pointer in image memory

        glCallList(dlist);
        //glFinish();
        SDL_GL_SwapBuffers();
    }
}
