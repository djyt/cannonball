# Default CMake Setup. Used for Mac OS x builds

# For INTeL 32bit Mac OS X 10.5+
set(CMAKE_CXX_FLAGS " -O2 -force_cpusubtype_ALL -mmacosx-version-min=10.5 -arch i386 -m32 -isysroot /Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5 -I/Developer/SDKs/MacOSX10.5.sdk/usr/X11R6/include/ ")

# # For PowerPC 32bit Mac OS X 10.5+
# set(CMAKE_CXX_FLAGS " -O2 -faltivec -force_cpusubtype_ALL -mmacosx-version-min=10.5 -arch ppc -m32 -isysroot /Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5 -I/Developer/SDKs/MacOSX10.5.sdk/usr/X11R6/include/ ")

# Use OpenGL for rendering
set(OPENGL 1)

# This is for MacPorts
set(lib_base /opt/local)
set(sdl_root ${lib_base})

include_directories("${sdl_root}/include/SDL/" "${lib_base}/include/" "/System/Library/Frameworks/" )

find_library(COCOA_LIBRARY Cocoa)
find_library(GLUT_LIBRARY GLUT )
find_library(OpenGL_LIBRARY OpenGL )

link_libraries(cannonball 
    SDL
    SDLmain
    ${COCOA_LIBRARY}
    ${GLUT_LIBRARY}
    ${OpenGL_LIBRARY}
)

# Linking
link_directories(
    "${sdl_root}/lib"
)
 
# Location for Cannonball to create save files
# Used to auto-generate setup.hpp with various file paths
set(xml_directory ./)
set(sdl_flags "SDL_DOUBLEBUF | SDL_SWSURFACE")
