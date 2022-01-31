# -----------------------------------------------------------------------------
# CannonBall Raspberry Pi
# (For local compilation)
# Raspberry Pi has full OpenGL support (with and w/o X11) via MESA and GLVND.
# -----------------------------------------------------------------------------

# Library Locations
set(boost_dir ${lib_base}/boost_1_74_0)
set(sdl2_dir ${lib_base}/SDL2-2.0.12)

# GCC Specific flags (optimize for Pi CPU with CFLAGS based on current Pi model,
# useful when building on the Pi itself instead of cross-compiling).
set(CMAKE_CXX_FLAGS "-O3 -march=native -mcpu=native -ftree-vectorize -pipe -fomit-frame-pointer")

# Platform Specific Libraries.

set(platform_link_libs
    SDL2
    OpenGL
)

# Platform Specific Link Directories
set(platform_link_dirs

)
