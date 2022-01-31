# -----------------------------------------------------------------------------
# CannonBall Raspberry Pi4 (Smarty Hardware)
# (For local compilation)
# -----------------------------------------------------------------------------

# Library Locations
set(boost_dir ${lib_base}/boost_1_74_0)
set(sdl2_dir ${lib_base}/SDL2-2.0.12)

# Use OpenGLES for rendering.
set(OPENGLES 1)

# GCC Specific flags (optimize for Pi CPU with CFLAGS based on current Pi model,
# useful when building on the Pi itself instead of cross-compiling).
set(CMAKE_CXX_FLAGS "-O3 -march=native -mcpu=native -ftree-vectorize -pipe -fomit-frame-pointer")

# Platform Specific Libraries
set(platform_link_libs
    SDL2
    GLESv2
)

# Platform Specific Link Directories
set(platform_link_dirs

)
