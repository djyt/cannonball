# Ubuntu Linux

set(lib_base /usr/include)
set(sdl_root ${lib_base}/SDL)

include_directories("${sdl_root}")

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
