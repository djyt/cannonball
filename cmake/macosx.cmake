# Default CMake Setup. Used for Mac OS x builds

# For INTeL 32bit Mac OS X 10.5+
set(CMAKE_CXX_FLAGS " -O2 -force_cpusubtype_ALL -arch i386 -m32 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk -mmacosx-version-min=10.7")

# # For PowerPC 32bit Mac OS X 10.5+
# set(CMAKE_CXX_FLAGS " -std=c++11 -stdlib=libstdc++ -O2 -faltivec -force_cpusubtype_ALL -mmacosx-version-min=10.5 -arch ppc -m32 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk -mmacosx-version-min=10.7")

# Use OpenGL for rendering
set(OPENGL 1)

# This is for Homebrew
set(lib_base /usr/local/Cellar)
set(sdl_root ${lib_base}/sdl/1.2.15)

include_directories("${sdl_root}/include/SDL/" "${lib_base}/include/" "~/Library/Frameworks/SDL.framework/Headers" "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk/usr/X11/include/" "${lib_base}/boost/1.60.0_1/include")

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
