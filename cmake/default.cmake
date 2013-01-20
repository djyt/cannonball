# Default CMake Setup. Used for Win32 Builds.

set(lib_base c:/coding/lib)
set(sdl_root ${lib_base}/SDL-1.2.15)

include_directories("${sdl_root}/include")

link_libraries(cannonball 
    SDL
    SDLmain
)

# Linking
link_directories(
    "${sdl_root}/lib"
)
 
# Location for Cannonball to create save files
# Used to auto-generate setup.hpp with various file paths
set(xml_directory ./)
set(sdl_flags "SDL_DOUBLEBUF | SDL_SWSURFACE")