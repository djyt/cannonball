# CMake setup used for SDL2 Builds.

set(lib_base /usr/include)
set(sdl_root ${lib_base}/SDL2)

include_directories("${sdl_root}")

link_libraries(cannonball 
    SDL2
    GL
)

# Linking
link_directories(
    "${sdl_root}/lib"
)

add_definitions(-O3 -DSDL2 -DWITH_OPENGL)
#add_definitions(-O0 -ggdb -DSDL2 -DWITH_OPENGL)
 
# Location for Cannonball to create save files
# Used to auto-generate setup.hpp with various file paths
set(xml_directory ./)
set(sdl_flags "SDL_WINDOW_RESIZABLE")

# Set SDL2 instead of SDL1
set(SDL2 1)
set(OPENGL 1)
