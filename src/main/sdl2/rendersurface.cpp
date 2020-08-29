/***************************************************************************
    SDL2 Hardware Surface Video Rendering
    Copyright Manuel Alfayate, Chris White.

    Copyright (c) 2020 James Pearce:
    - Blargg CRT filter integration
    - CRT Masks - shadow or aperture-grille
    - Scanlines - base image or screen resolution
    - Round corner mask
    - Vignette mask
    - RGB colour processing:
      - Offset (adds a fixed value)
      - Floor (sets the minimum value)
      - Overdrive (maxes out above a certain value)
      - Bloom (bleeds brightness into neighbouring pixels)
      - Scale (multiplies)
      - Power (gamma)

    See license.txt for more details.
***************************************************************************/

#include <iostream>
#include "rendersurface.hpp"
#include "frontend/config.hpp"

// Blargg filter for CRT processing (JJP) - with SMP support
#include "snes_ntsc.h"
#include <omp.h>

// JJP - Blargg filtering for CRT style visuals
static snes_ntsc_setup_t setup;
static snes_ntsc_t* ntsc = 0;
static int shader;                          // true if using Blargg filter
static int vignette;                        // true if vignette is to be applied to screen
static int flicker;                         // true is screen flicker should be added with vignette
static uint16_t *rgb_pixels = 0;            // used by Blargg filter
static uint16_t *filtered_rgb_pixels = 0;   // used by Blargg filter for RGB output
static uint32_t *filtered_pixels = 0;       // used by Blargg filter for native output
static double *vignette_mask = 0;           // dims image intensity towards edges
static int phase;
static int phaseframe;
static int snes_src_width;
static int framesrendered;
static int framesdropped;
static uint32_t framenumber;
static int Alevel = 192;                    // default alpga value for game image
static int crtmask = 0;
static int mask_intensity = 0;
static uint32_t screen_r[256]; // rainbow table to lookup exponent values for red channel
static uint32_t screen_g[256]; // rainbow table to lookup exponent values for green channel
static uint32_t screen_b[256]; // rainbow table to lookup exponent values for blue channel
static int alpha_target = 0xff;
static uint32_t *game_pixels;
static uint32_t *overlay_pixels;



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

    alpha_target = int( double(config.video.brightness) * 255.0 / 100.0 ) ; // used for final intensity overlay blend

    // JJP - Blargg filtering
    (config.video.blargg > video_settings_t::BLARGG_DISABLE) ? shader = 1 : shader = 0; // enable Blargg based on settings
    (config.video.vignette) ? vignette = 1 : vignette = 0;
    (config.video.flicker) ? flicker = 1 : flicker = 0;
    crtmask = config.video.mask;
    mask_intensity = config.video.mask_strength;
    if (shader) {
        // first calculate the resultant image size.
        snes_src_width = SNES_NTSC_OUT_WIDTH( src_width );
        if (config.video.hires) snes_src_width = snes_src_width >> 1;
        snes_src_width++; // not sure why!
        if (!ntsc) ntsc = (snes_ntsc_t*) malloc( sizeof(snes_ntsc_t) );

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
        phase = 0;                      // initial frame will be phase 0
        phaseframe = 0;                 // used to track phase at 60 fps
        Alevel = 255;                   // blackpoint
        setup.hue        = 0; //double(config.video.hue) / 100;
        setup.saturation = double(config.video.saturation) / 100;
        setup.contrast   = double(config.video.contrast) / 100;
        setup.brightness = 0; //double(config.video.brightness) / 100;
        setup.sharpness  = double(config.video.sharpness) / 100;
        setup.gamma      = double(config.video.gamma) / 100;
        //setup.bleed      = config.video.bleed;
        //setup.fringing = 1;
        snes_ntsc_init( ntsc, &setup ); // configure the library
    }

    // Setup SDL Screen size
    if (!RenderBase::sdl_screen_size())
        return false;

    int flags = SDL_FLAGS;

    // In SDL2, we calculate the output dimensions, but then in draw_frame() we won't do any scaling: SDL2
    // will do that for us, using the rects passed to SDL_RenderCopy().
    // scn_* -> physical screen dimensions OR window dimensions. On FULLSCREEN MODE it has the physical screen
    //		dimensions and in windowed mode it has the window dimensions.
    // src_* -> real, internal, frame dimensions. Will ALWAYS be 320 or 398 x 224, unless using the Blargg CRT
    // filter, in which case the source will be snes resolution computed by SNES_NTSC_OUT_WIDTH( src_width )
    // above corrected_scn_width_* -> output screen size for scaling.
    // In windowed mode it's the size of the window.

    // --------------------------------------------------------------------------------------------
    // Full Screen Mode
    // --------------------------------------------------------------------------------------------
    printf("src_width: %i\nsrc_height: %i\n",src_width,src_height);
    if (video_mode == video_settings_t::MODE_FULL || video_mode == video_settings_t::MODE_STRETCH)
    {
	flags |= (SDL_WINDOW_FULLSCREEN); // Set SDL flag

	// Fullscreen window size: SDL2 creates a full-screen window at the configured monitor resolution.
	scn_width  = orig_width;
        scn_height = orig_height;

	(shader) ? src_rect.w = snes_src_width : src_rect.w = src_width;
	src_rect.h = src_height;
	src_rect.x = 0;
	src_rect.y = 0;

        // configure display ratio
        // game image is either:
        // - 320 x 224 (original)
        // - 398 x 224 (widescreen)

	dst_rect.w = scn_width;
	dst_rect.h = int(uint32_t(src_height) * uint32_t(scn_width) / uint32_t(src_width));
	dst_rect.y = (scn_height - dst_rect.h) >> 1; // centre on screen
	dst_rect.x = 0;
        if (dst_rect.h > scn_height) {
            // too large; proportion the other way
            dst_rect.w = int(uint32_t(src_width) * uint32_t(scn_height) / uint32_t(src_height));
            dst_rect.h = scn_height;
            dst_rect.x = (scn_width - dst_rect.w) >> 1; // centre on screen
            dst_rect.y = 0;
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

	(shader) ? src_rect.w = snes_src_width : src_rect.w = src_width;
	src_rect.h = src_height;
	src_rect.x = 0;
	src_rect.y = 0;
	dst_rect.w = scn_width;
	dst_rect.h = scn_height;
	dst_rect.x = 0;
	dst_rect.y = 0;

        SDL_ShowCursor(true);
    }

    scanline_rect.w = 1; //src_rect.w;
    scanline_rect.h = (config.video.hires) ? src_rect.h << 1 : src_rect.h << 2;
    scanline_rect.x = 0;
    scanline_rect.y = 0;

    rgb_rect.w = dst_rect.w;
    rgb_rect.h = dst_rect.h;
    rgb_rect.x = 0;
    rgb_rect.y = 0;

    //int bpp = info->vfmt->BitsPerPixel;
    const int bpp = 32;

    // Frepare new surface
    if (surface)
        SDL_FreeSurface(surface);
    surface = SDL_CreateRGBSurfaceWithFormat(0, src_rect.w, src_rect.h, bpp, SDL_PIXELFORMAT_ARGB8888);
    if (!surface)
    {
        std::cerr << "Surface creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Get SDL Pixel Format Information
    Rshift = surface->format->Rshift;
    Gshift = surface->format->Gshift;
    Bshift = surface->format->Bshift;
    Ashift = surface->format->Ashift;
    Rmask  = surface->format->Rmask;
    Gmask  = surface->format->Gmask;
    Bmask  = surface->format->Bmask;
    Amask  = surface->format->Amask;

    // Create window with SDL and set up the stacked textures
    if (config.video.filtering==0) SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    else SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    window = SDL_CreateWindow("Cannonball", 0, 0, scn_width, scn_height, flags);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);//|SDL_RENDERER_PRESENTVSYNC);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");

    last_game_tx = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               src_rect.w, src_rect.h);
    game_tx = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               src_rect.w, src_rect.h);
    scanline_tx = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STATIC,
                               scanline_rect.w, scanline_rect.h);
    rgb_tx1 = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STATIC,
                               rgb_rect.w, rgb_rect.h);
    overlay = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                              src_rect.w, src_rect.h);
    vignette_tx = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STATIC,
                               dst_rect.w, dst_rect.h); // hw display pixels covered by image
//    SDL_SetTextureBlendMode(game_tx, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(rgb_tx1, SDL_BLENDMODE_MOD);
    SDL_SetTextureBlendMode(scanline_tx, SDL_BLENDMODE_MOD);
    SDL_SetTextureBlendMode(overlay, SDL_BLENDMODE_ADD);
    SDL_SetTextureBlendMode(vignette_tx, SDL_BLENDMODE_MOD);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // create the last frame, as to start with that is obviously black
    uint32_t *texture_pixels = new uint32_t[(src_rect.w * src_rect.h)];
    memset(texture_pixels, 0x00, sizeof(uint32_t[(src_rect.w * src_rect.h)]));
    // load into SDL
    SDL_UpdateTexture(last_game_tx, NULL, texture_pixels, src_rect.w * sizeof(Uint32));
    delete[] texture_pixels;

    // create the horizontal scanline texture and prepare it
    texture_pixels = new uint32_t[(scanline_rect.w * scanline_rect.h)];
    memset(texture_pixels, 0xff, sizeof(uint32_t[(scanline_rect.w * scanline_rect.h)])); // fill with white i.e. no dimming
    if (scanlines>0) {
        uint32_t *scnlp = texture_pixels;
        uint32_t dimval; uint32_t rgbdim;
        dimval = ((uint32_t(100-scanlines) * 255 / 100) & 0xff);
        rgbdim = (dimval << Rshift) + (dimval << Gshift) + (dimval << Bshift);
        for (int y=0; y < scanline_rect.h; y++) {
            for (int x=0; x < scanline_rect.w; x++) {
                if ((y & 0x03)==3) *scnlp = rgbdim;
                scnlp++;
            }
        }
    }
    // load into SDL, even if it's just blank
    SDL_UpdateTexture(scanline_tx, NULL, texture_pixels, scanline_rect.w * sizeof(Uint32));
    delete[] texture_pixels;

    // create the rgb texture(s) and prepare it
    // various mask designs
    texture_pixels = new uint32_t[(rgb_rect.w * rgb_rect.h)];
    memset(texture_pixels, 0xff, sizeof(uint32_t[(rgb_rect.w * rgb_rect.h)])); // fill with white i.e. no dimming
    if (crtmask) {
        uint32_t *scnlp = texture_pixels;
        uint32_t dimval;
        if (crtmask==1) {
            dimval = ((uint32_t(100-mask_intensity) * 255 / 100) & 0xFF) << Ashift;
            // approximation to slot-mask arcade screen, needs 4K screen really
            for (int y=0; y<rgb_rect.h; y++) {
                for (int x=0; x<rgb_rect.w; x++) {
                    // first, create the RGB columns
                    if ((x & 0x03)==0) *scnlp = (0xff << Rshift) + (dimval << Gshift) + (dimval << Bshift);   // red column
                    if ((x & 0x03)==1) *scnlp = (dimval << Rshift) + (0xff << Gshift) + (dimval << Bshift);   // green column
                    if ((x & 0x03)==2) *scnlp = (dimval << Rshift) + (dimval << Gshift) + (0xff << Bshift);   // blue column
                    if ((x & 0x03)==3) *scnlp = (dimval << Rshift) + (dimval << Gshift) + (dimval << Bshift); // mask column (dark)
                    // next, create the scan lines, though these are offset across each frame of the interlace
                    if ((y & 0x03)==1) {
                        // dim alternate pixels on this whole row
                        if ((x & 0x07) <= 2) *scnlp = (dimval << Rshift) + (dimval << Gshift) + (dimval << Bshift);
                    }
                    if ((y & 0x03)==3) {
                        // dim alternate pixels on this whole row
                        if ((x & 0x07) >= 4) *scnlp = (dimval << Rshift) + (dimval << Gshift) + (dimval << Bshift);
                    }
                    scnlp++;
                }
            }
        }
        else if (crtmask==2) {
            // diagonal rgb pattern; softens edges, not based on anything real
            int offsets [4] = {0,1,2,3}; int current_offset = 0; int current = 0;
            //memset(texture_pixels, 0xff, sizeof(uint32_t[(rgb_rect.w * rgb_rect.h)])); // fill with white
            dimval = ((uint32_t(100-mask_intensity) * 255 / 100) & 0xFF);
            for (int y=0; y<rgb_rect.h; y++) {
                current = offsets[current_offset];
                for (int x=0; x<rgb_rect.w; x++) {
                    // first, create the columns
                    if (current==0) *scnlp = (0xff << Rshift) + (dimval << Gshift) + (dimval << Bshift); // red column
                    if (current==1) *scnlp = (dimval << Rshift) + (0xff << Gshift) + (dimval << Bshift); // green column
                    if (current==2) *scnlp = (dimval << Rshift) + (dimval << Gshift) + (0xff << Bshift); // blue column
                    scnlp++;
                    if (++current==3) current = 0;
                }
                if (++current_offset==3) current_offset = 0;
            }
        }
        else if (crtmask==3) {
            // straight rgb lines
            int offsets [4] = {0,0,0,0}; int current_offset = 0; int current = 0;
            //memset(texture_pixels, 0xff, sizeof(uint32_t[(rgb_rect.w * rgb_rect.h)])); // fill with white
            dimval = ((uint32_t(100-mask_intensity) * 255 / 100) & 0xFF);
            for (int y=0; y<rgb_rect.h; y++) {
                current = offsets[current_offset];
                for (int x=0; x<rgb_rect.w; x++) {
                    // first, create the columns
//                    if (current==0) *scnlp = (dimval << Ashift) + (0xff << Rshift); // red column
//                    if (current==1) *scnlp = (dimval << Ashift) + (0xff << Gshift); // green column
//                    if (current==2) *scnlp = (dimval << Ashift) + (0xff << Bshift); // blue column
                    if (current==0) *scnlp = (0xff << Rshift) + (dimval << Gshift) + (dimval << Bshift); // red column
                    if (current==1) *scnlp = (dimval << Rshift) + (0xff << Gshift) + (dimval << Bshift); // green column
                    if (current==2) *scnlp = (dimval << Rshift) + (dimval << Gshift) + (0xff << Bshift); // blue column
                    scnlp++;
                    if (++current==3) current = 0;
                }
                if (++current_offset==4) current_offset = 0;
            }
        }
        else if (crtmask==4) {
            // straight verticle dimmed lines
            int offsets [4] = {0,0,0,0}; int current_offset = 0; int current = 0;
            //memset(texture_pixels, 0xff, sizeof(uint32_t[(rgb_rect.w * rgb_rect.h)])); // fill with white
            dimval = ((uint32_t(100-mask_intensity) * 255 / 100) & 0xFF);
            for (int y=0; y<rgb_rect.h; y++) {
                current = offsets[current_offset];
                for (int x=0; x<rgb_rect.w; x++) {
                    if (current==3) *scnlp = (dimval << Rshift) + (dimval << Gshift) + (dimval << Bshift); // dark column
                    scnlp++;
                   if (++current==4) current = 0;
                }
                if (++current_offset==4) current_offset = 0;
            }
        }
        else if (crtmask==5) {
            // small square mask effect but without rgb split, for standard screens like 1280x1024
            int offsets [4] = {0,0,0,0}; int current_offset = 0; int current = 0;
            //memset(texture_pixels, 0xff, sizeof(uint32_t[(rgb_rect.w * rgb_rect.h)])); // fill with white
            int dimval_h = ((uint32_t(100-mask_intensity) * 255 / 100) & 0xFF);
            int dimval_v = dimval_h << 1;
            for (int y=0; y<rgb_rect.h; y++) {
                current = offsets[current_offset];
                for (int x=0; x<rgb_rect.w; x++) {
                    if ((current == 2) || (current == 5)) *scnlp = (dimval_v << Rshift) + (dimval_v << Gshift) + (dimval_v << Bshift); // dark column
                    if ((y & 0x01)==0) {
                        // dim alternate pixels on this whole row
                        if (current < 2) *scnlp = (dimval_h << Rshift) + (dimval_h << Gshift) + (dimval_h << Bshift);
                    }
                    if ((y & 0x01)==1) {
                        // dim alternate pixels on this whole row
                        if ((current > 2) && (current < 5)) *scnlp = (dimval_h << Rshift) + (dimval_h << Gshift) + (dimval_h << Bshift);
                    }
                    scnlp++;
                    if (++current==6) current = 0;
                }
                if (++current_offset==4) current_offset = 0;
            }
        }
        else if (crtmask==6) {
            // small shadow mask style (no rgb, for standard screens)
            int offsets [4] = {0,3,0,3}; int current_offset = 0; int current = 0;
            // memset(texture_pixels, 0xff, sizeof(uint32_t[(rgb_rect.w * rgb_rect.h)])); // fill with white
            dimval = ((uint32_t(100-mask_intensity) * 255 / 100) & 0xFF);
            int dimval2 = dimval;
            for (int y=0; y<rgb_rect.h; y++) {
                current = offsets[current_offset];
                for (int x=0; x<rgb_rect.w; x++) {
                    if (current==0) *scnlp = (0xff << Rshift) + (dimval << Gshift) + (dimval << Bshift); // red column
                    if (current==1) *scnlp = (dimval << Rshift) + (0xff << Gshift) + (dimval << Bshift); // green column
                    if (current==2) *scnlp = (dimval << Rshift) + (dimval << Gshift) + (0xff << Bshift); // blue column
                    if (current==3) *scnlp = (0xff << Rshift) + (dimval2 << Gshift) + (dimval2 << Bshift); // red column
                    if (current==4) *scnlp = (dimval2 << Rshift) + (0xff << Gshift) + (dimval2 << Bshift); // green column
                    if (current==5) *scnlp = (dimval2 << Rshift) + (dimval2 << Gshift) + (0xff << Bshift); // blue column
                    scnlp++;
                    if (++current==6) current = 0;
                }
                if (++current_offset==4) current_offset = 0;
            }
        }
        else if (crtmask==7) {
            // larger shadow mask effect
            dimval = ((uint32_t(100-mask_intensity) * 255 / 100) & 0xff);
            int dimval2 = dimval;
            int current;
            // close simulation to actual arcade CRT. Needs a 4K screen.
            for (int y=0; y<rgb_rect.h; y++) {
                current = 0;
                for (int x=0; x<rgb_rect.w; x++) {
                    // first, create the RGB columns
                    if (current==0) *scnlp = (0xff << Rshift) + (dimval << Gshift) + (dimval << Bshift); // red column
                    if (current==1) *scnlp = (dimval << Rshift) + (0xff << Gshift) + (dimval << Bshift); // green column
                    if (current==2) *scnlp = (dimval << Rshift) + (dimval << Gshift) + (0xff << Bshift); // blue column
                    if (current==3) *scnlp = (0xff << Rshift) + (dimval << Gshift) + (dimval << Bshift); // red column
                    if (current==4) *scnlp = (dimval << Rshift) + (0xff << Gshift) + (dimval << Bshift); // green column
                    if (current==5) *scnlp = (dimval << Rshift) + (dimval << Gshift) + (0xff << Bshift); // blue column
//                    if ((x & 0x03)==0) *scnlp = dimval + (0xff << Rshift); // red column
//                    if ((x & 0x03)==1) *scnlp = dimval + (0xff << Gshift); // green column
//                    if ((x & 0x03)==2) *scnlp = dimval + (0xff << Bshift); // blue column
                    // next, create the scan lines, though these are offset across each frame of the interlace
                    if ((y & 0x03)==1) {
                        // dim alternate pixels on this whole row
                        if (current <= 2)
                             *scnlp = (dimval2 << Rshift) + (dimval2 << Gshift) + (dimval2 << Bshift);
                    }
                    if ((y & 0x03)==3) {
                        // dim alternate pixels on this whole row
                        if (current >= 3)
                             *scnlp = (dimval2 << Rshift) + (dimval2 << Gshift) + (dimval2 << Bshift);
                    }
                    scnlp++;
                    if (++current==6) current = 0;
                }
            }
        }
    }
    // load into SDL, even if it's just blank
    SDL_UpdateTexture(rgb_tx1, NULL, texture_pixels, rgb_rect.w * sizeof(Uint32));
    delete[] texture_pixels;

    // create vignette filter and texture
    texture_pixels = new uint32_t[(dst_rect.w * dst_rect.h)];
    memset(texture_pixels, 0xff, sizeof(uint32_t[(dst_rect.w * dst_rect.h)])); // fill with white
    if (vignette) {
        // next create a native resolution filter to create rounded corners on the screen
        // the interaction with the above game filter won't be perfect due to image ratio differences
        // and alpha blend vs value multiplication
        uint32_t vignette_target = round( ( (double(config.video.vignette) / 100.0) * 255.0 ) );
        uint32_t *scnlp = texture_pixels;
        double midx = double(dst_rect.w >> 1);
        double midy = double(dst_rect.h >> 1);
        double dia = sqrt( ((midx * midx) + (midy * midy)) );
        double outer = dia * 0.98;
        double inner = dia * 0.30;
        uint32_t shadeval;
        double total_black = 0.0; double d;
        for (int y=0; y<dst_rect.h; y++) {
            for (int x=0; x<dst_rect.w; x++) {
                // calculate distance to middle
                d = sqrt( ((midx - double(x)) * (midx - double(x))) + ((midy - double(y)) * (midy - double(y))) );
                if (d < 0.0) d = d * -1.0; // create absolute distance
                if (d >= outer) {
                   // black out beyond this region
                   shadeval = total_black;
                } else if (d >= inner) {
                    // intermediate value; increase intensity with square of distance to avoid visible edge
                    shadeval = 255 - uint32_t(round( ((vignette_target) * ((d - inner) * (d - inner)) /
                                                 ((outer - inner) * (outer - inner))) ));
                } else shadeval = 0xff; // no dimming
                *scnlp++ = (shadeval << Rshift) + (shadeval << Bshift) + (shadeval << Gshift);
            }
        }
    }
    // load the texture into SDL, even if it's just blank
    SDL_UpdateTexture(vignette_tx, NULL, texture_pixels, dst_rect.w * sizeof(Uint32));
    delete[] texture_pixels;

/*    // create reflection filter and texture
    texture_pixels = new uint32_t[(dst_rect.w * dst_rect.h)];
    memset(texture_pixels, 0x00, sizeof(uint32_t[(dst_rect.w * dst_rect.h)])); // fill with black
    if (vignette) {
        // next create a native resolution filter to add a splodge of white
        uint32_t *scnlp = texture_pixels;
        double midx = double(dst_rect.w >> 1);
        double midy = double(dst_rect.h >> 1);
        double splodge_x = midx * 1.3;
        double splodge_y = midy / 1.3;
        double dia = sqrt( ((midx * midx) + (midy * midy)) );
        double outer = dia * 0.03;
        double inner = dia * 0.005;
        uint32_t shadeval;
        double d;
        for (int y=0; y<dst_rect.h; y++) {
            for (int x=0; x<dst_rect.w; x++) {
//        for (int y=start_y; y < end_y; y++) {
//            for (int x=start_x; x < end_x; x++) {
                // calculate distance to middle of splodge
                d = sqrt( ((splodge_x - double(x)) * (splodge_x - double(x))) +
                          ((splodge_y - double(y)) * (splodge_y - double(y))) );
                if (d < 0.0) d = d * -1.0; // create absolute distance
                if (d >= outer) {
                   // add nothing in this region - beyond the outside circle
                   shadeval = 0x00;
                } else if (d >= inner) {
                    // intermediate value; increase intensity with square of distance to avoid visible edge
                    shadeval = 0xff - uint32_t( round( (0xff * ((d - inner) * (d - inner))) /
                                                       ((outer - inner) * (outer - inner)) ) );
                } else shadeval = 0xff; // white out the centre itself
                *scnlp++ = (shadeval << Rshift) + (shadeval << Bshift) + (shadeval << Gshift) + (128 << Ashift);
            }
        }
    }
    // load the texture into SDL, even if it's just blank
    SDL_UpdateTexture(reflection_tx, NULL, texture_pixels, dst_rect.w * sizeof(Uint32));
    delete[] texture_pixels;
*/
    // create some other buffers
    delete[] rgb_pixels;
    rgb_pixels = new uint16_t[src_width * src_height];
    delete[] filtered_rgb_pixels;
    filtered_rgb_pixels = new uint16_t[(snes_src_width+1) * src_height];
    delete[] filtered_pixels;
    filtered_pixels = new uint32_t[(snes_src_width+1) * src_height];

    // Convert the SDL pixel surface to 32 bit.
    // This is potentially a larger surface area than the internal pixel array.
    screen_pixels = (uint32_t*)surface->pixels;

    // pre-calculate the rgb multipliers based on configured color curves. These
    // will be used every frame, and floaring point exponents are expensive!
    int bleed;
    int bleedlimit = 255;
    for (int x=0; x<=255; x++) {
        screen_r[x] = uint32_t( pow((float(x)/255.0),1.75) * 255.0 * 1.0 );
        screen_g[x] = uint32_t( pow((float(x)/255.0),1.75) * 255.0 * 1.0 );
        screen_b[x] = uint32_t( pow((float(x)/255.0),1.5) * 255.0 * 1.2 );
        if (screen_r[x] > bleedlimit) {
            bleed = screen_r[x] - bleedlimit;
            screen_g[x] = screen_g[x] + bleed;
            screen_b[x] = screen_b[x] + bleed;
        }
        if (screen_g[x] > bleedlimit) {
            bleed = screen_g[x] - bleedlimit;
            screen_r[x] = screen_r[x] + bleed;
            screen_b[x] = screen_b[x] + bleed;
        }
        if (screen_b[x] > bleedlimit) {
            bleed = screen_b[x] - bleedlimit;
            screen_r[x] = screen_r[x] + bleed;
            screen_g[x] = screen_g[x] + bleed;
        }
        screen_r[x] = (screen_r[x] > 255) ? 255 : screen_r[x];
        screen_g[x] = (screen_g[x] > 255) ? 255 : screen_g[x];
        screen_b[x] = (screen_b[x] > 255) ? 255 : screen_b[x];
    }

    delete[] game_pixels; // don't leak!
    delete[] overlay_pixels;
    game_pixels = new uint32_t[src_rect.w * src_rect.h];
    overlay_pixels = new uint32_t[src_rect.w * src_rect.h];

    return true;
}

void RenderSurface::disable()
{
    SDL_DestroyTexture(last_game_tx);
    SDL_DestroyTexture(game_tx);
    SDL_DestroyTexture(scanline_tx);
    SDL_DestroyTexture(rgb_tx1);
    SDL_DestroyTexture(vignette_tx);
    SDL_DestroyTexture(overlay);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

bool RenderSurface::start_frame()
{
    return true;
}


void RenderSurface::colorise_frame(int ThreadID, int TotalThreads,
                                   uint32_t *gameimage, uint32_t *coloredimage, uint32_t *intensityimage,
                                   int width, int height)
{
    int block_rows = height / TotalThreads;
    int first_block_rows = block_rows;

    if (ThreadID==(TotalThreads-1)) {
         // add any remainder to the last block processed
        block_rows += (height % TotalThreads);
    }
    // calculate how much data to process in this thread
    int start = width * first_block_rows * ThreadID;
    int end   = start + (width * block_rows);

    gameimage += start;
    coloredimage += start;
    intensityimage += start;

//    float phasedim = 1.0;
//    float red; float green; float blue;
    uint32_t red; uint32_t green; uint32_t blue;
    uint32_t red_blend, green_blend, blue_blend;
    uint32_t intensity;
    for (int y=0; y < block_rows; y++) {
        for (int x=0; x < width; x++) {
            // CRT colour curve mapping
            red   = (*gameimage & Rmask) >> Rshift; //screen_r[(*gameimage & Rmask) >> Rshift];
            green = (*gameimage & Gmask) >> Gshift; //screen_g[(*gameimage & Gmask) >> Gshift];
            blue  = (*gameimage & Bmask) >> Bshift; //screen_b[(*gameimage & Bmask) >> Bshift]; *
            *coloredimage = ( (red << Rshift) +     // Red
                              (green << Gshift) +  // Green
                              (blue << Bshift) +    // Blue
                              (0xff << Ashift) );   // A
            intensity        = ((red + green + blue) >> 2) & 0xff;
            *intensityimage  = ( (intensity << Rshift) +
                                 (intensity << Gshift) +
                                 (intensity << Bshift) +
                                 (0xff << Ashift) );
            // advance pointers
            gameimage++; coloredimage++; intensityimage++;
        }
    }
}


bool RenderSurface::finalize_frame()
{
    int game_width = src_rect.w;
    int game_height = src_rect.h;

    uint32_t *srcpix = (shader==0) ? screen_pixels : filtered_pixels;
    void *mpixels; int mpitch;
    uint32_t cpybytes;

    int ID;
    int threads = omp_get_max_threads();
    omp_set_num_threads(threads);

    if (shader) {
        // Blargg filtering provides more of a CRT look.
        framenumber++;
        // process this in specified number of threads
        #pragma omp parallel
        {
            ID = omp_get_thread_num();
            shade(ID,threads);
        }

        // update phase.  We need to consider whether we're running at 30 or 60 fps
        if (config.fps == 60) {
            if (++phaseframe == 2) {
                phase ^= 1;
                phaseframe = 0;
            }
        } else phase ^= 1; // 0 -> 1 -> 0 -> 1...
    }

    // CPU processing of underlying game texture is floating point heavy, so split into threads
    // per the Blargg setting for the same
    #pragma omp parallel
    {
        ID = omp_get_thread_num();
        colorise_frame(ID,threads,srcpix,game_pixels,overlay_pixels,game_width,game_height);
    }

//    SDL_RenderCopy(renderer, last_game_tx, &src_rect, &dst_rect);         // previous frame becomes the underlay
    SDL_UpdateTexture(game_tx, NULL, game_pixels, game_width * sizeof (Uint32));
    SDL_SetTextureColorMod(game_tx, 0xff, 0xff, 0xff); // rgb balance
//    SDL_SetTextureAlphaMod(game_tx, 224); // represent over the top of the previous frame (phosphor persistence)

    SDL_UpdateTexture(overlay, NULL, overlay_pixels, game_width * sizeof (Uint32));
    SDL_SetTextureAlphaMod(overlay, alpha_target); // bloom intensity

    // now pass the layers to the GPU to deal with
    SDL_RenderClear(renderer);

//    SDL_RenderCopy(renderer, last_game_tx, &src_rect, &dst_rect);   // old game image underlay
    SDL_RenderCopy(renderer, game_tx, &src_rect, &dst_rect);        // game image, maybe with CRT filter for artifacts
//    SDL_RenderCopy(renderer, game_tx, &src_rect, &dst_rect);      // game image + game image (brightness)

    if (crtmask) {
        SDL_RenderCopy(renderer, rgb_tx1, &rgb_rect, &dst_rect);       // vertical rgb overlay
        SDL_RenderCopy(renderer, overlay, &src_rect, &dst_rect);       // brightens and highlights
    }
    if (scanlines) SDL_RenderCopy(renderer, scanline_tx, &scanline_rect, &dst_rect);// scanline overlay
    if (crtmask)   SDL_RenderCopy(renderer, overlay, &src_rect, &dst_rect);         // brightens and highlights, second pass
    if (vignette)  SDL_RenderCopy(renderer, vignette_tx, &rgb_rect, &dst_rect);     // vignette overlay comes last (on top)

    // update the screen
    SDL_RenderPresent(renderer);

//    and copy the current game image into the previous game image texture
//    SDL_UpdateTexture(last_game_tx, NULL, game_pixels, game_width * sizeof (Uint32));

    return true;
}

void RenderSurface::shade(int ThreadID, int TotalThreads) //, uint16_t* pixels)
{
    // Does converts the native screen image storage in 'pixels' array to Blargg filtered image
    // in filtered_pixels (global).
    register uint32_t current_val;
    register uint16_t current_rgb_val;
//    uint16_t* current_pixel = pixels;
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
//    current_pixel += start;
//    th_rgb_pixels += start;

    long output_pitch = (snes_src_width << 1); // 2 bytes-per-pixel (5/6/5)
    th_rgb_pixels = rgb_pixels + start; // move back to start of data processed
    start = (output_pitch >> 1) * first_block_rows * ThreadID; // start position in filtered array
    end   = start + ((output_pitch >> 1) * block_rows);        // ..and the end position
    th_filtered_rgb_pixels = filtered_rgb_pixels + start;
    th_filtered_pixels = filtered_pixels + start;

//...
    // Now call the blargg code, to do the work...
    if (config.video.hires) {
        snes_ntsc_blit_hires( ntsc, th_rgb_pixels, long(src_width), phase,
                              src_width, block_rows, th_filtered_rgb_pixels, output_pitch );
    } else {
        // standard res processing
        snes_ntsc_blit( ntsc, th_rgb_pixels, long(src_width), phase,
                        src_width, block_rows, th_filtered_rgb_pixels, output_pitch );
    }

    // finally, convert pixel data back to expected format
    for (int i = start; i < end; i++) {
        current_rgb_val = *(th_filtered_rgb_pixels++);
        *(th_filtered_pixels++) = uint32_t( (uint32_t((current_rgb_val >> 8) & 0x00F8) << Rshift) +   // Red
                                            (uint32_t((current_rgb_val >> 3) & 0x00FC) << Gshift) +   // Green
                                            (uint32_t((current_rgb_val << 3) & 0x00F8) << Bshift) +   // Blue
                                            (uint32_t(Alevel) << Ashift) );                           // A
    }
}



void RenderSurface::draw_frame(uint16_t* pixels)
{
    // grabs the S16 frame buffer (pixels) and stores it, either
    // as straight SDL RGB or SNES RGB ready for Blargg filter, if enabled
    int threads = omp_get_max_threads();
    omp_set_num_threads(threads);
    framesrendered++;
    if (shader) {
        // convert pixel data to format used by Blarrg filtering code
        #pragma omp parallel
        {
            uint16_t* tpix = pixels;
            uint16_t* spix = rgb_pixels;
            uint32_t current_val;
            #pragma omp parallel for
            for (int i = 0; i < (src_width * src_height); i++) {
                // first, convert the game generated image to RGB as per the standard display code
                current_val = rgb[*(tpix++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
                // then convert that ready for Blargg
                *(spix++) = uint16_t( ((uint16_t((current_val & Rmask) >> Rshift) & 0x00F8) << 8) +         // red
                                            ((uint16_t((current_val & Gmask) >> Gshift) & 0x00FC) << 3) +   // green
                                            ((uint16_t((current_val & Bmask) >> Bshift) & 0x00F8) >> 3) );  // blue
            }
        }
    } else {
        // Standard image processing; lookup real RGB value from rgb array for backbuffer
        #pragma omp parallel
        {
            uint16_t* tpix = pixels;
            uint32_t* spix = screen_pixels;
            #pragma omp parallel for
            for (int i = 0; i < (src_width * src_height); i++)
                *(spix++) = rgb[*(tpix++) & ((S16_PALETTE_ENTRIES * 3) - 1)] +      // RGB
                          (uint32_t(Alevel) << Ashift);                             // A
        }
    }
}
