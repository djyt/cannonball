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

# Use OpenGLES for rendering.
set(RENDER_ENGINE OpenGLES)

# GCC Specific flags (optimize for Pi4 CPU)
#set(CMAKE_CXX_FLAGS "-O3 -mtune=cortex-a72")

# Platform Specific Libraries
set(platform_link_libs
)

# Platform Specific Link Directories
set(platform_link_dirs
)
