# -----------------------------------------------------------------------------
# CannonBall Raspberry Pi4 (Smarty Hardware)
# -----------------------------------------------------------------------------

# Library Locations
set(lib_base c:/coding/lib)
set(boost_dir ${lib_base}/boost_1_74_0)
set(sdl2_dir ${lib_base}/SDL2-2.0.12)

# Use OpenGLES for rendering.
set(OPENGLES 1)

# Platform Specific Libraries
set(platform_link_libs
    SDL2
    GLESv2
)

# Platform Specific Link Directories
set(platform_link_dirs

)