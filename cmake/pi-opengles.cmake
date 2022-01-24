# -----------------------------------------------------------------------------
# CannonBall Raspberry Pi4 (Smarty Hardware)
#
# I am using Visual Studio 2019 and cross-compiling on a PC with VisualGDB 
# (https://visualgdb.com/) 
#
# This works well in terms of being able to use the Visual Studio debugger,
# but if your environment is different, you may need a different cmake file.
#
# I referred to the following tutorial:
# https://gnutoolchains.com/raspberry/tutorial/
#
# I downloaded the 2019-07-10 raspbian-buster-full executable from here:
# https://gnutoolchains.com/raspberry/
# -----------------------------------------------------------------------------

# Library Locations
set(lib_base c:/coding/lib)
set(boost_dir ${lib_base}/boost_1_74_0)
set(sdl2_dir ${lib_base}/SDL2-2.0.12)

# Use OpenGLES for rendering.
set(OPENGLES 1)

# GCC Specific flags (optimize for Pi1/2/3/4 CPU, depending on where it's being built.)
set(CMAKE_CXX_FLAGS "-O3 -march=native -mcpu=native -O2 -ftree-vectorize -pipe -fomit-frame-pointer")

# Platform Specific Libraries
set(platform_link_libs
    SDL2
    GLESv2
)

# Platform Specific Link Directories
set(platform_link_dirs

)
