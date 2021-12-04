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

# Use OpenGL for rendering.
find_package(OpenGL QUIET)
IF(OpenGL_FOUND)
        message(STATUS "Using OpenGL GLX for OpenGL")
ELSE(OpenGL_FOUND)
        # If we couldn't find old GLX package, try to use modern libglvnd libOpenGL.so instead,
        # which is the way to have full OpenGL without any X11 dependencies.
        message(STATUS "Using GLVND for OpenGL")
        FIND_LIBRARY(OPENGL_LIBRARIES OpenGL)
ENDIF(OpenGL_FOUND)

# GCC Specific flags (optimize for Pi4 CPU)
set(CMAKE_CXX_FLAGS "-O3 -mtune=cortex-a72")

# Platform Specific Libraries
set(platform_link_libs
    SDL2
    ${OPENGL_LIBRARIES}
)

# Platform Specific Link Directories
set(platform_link_dirs

)
