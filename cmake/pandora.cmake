# Default CMake Setup. Used for Pandora Builds.

set(lib_base /mnt/utmp/codeblocks/usr)
set(sdl_root ${lib_base}/include/SDL)

include_directories("${sdl_root}")

link_libraries(cannonball 
    SDL
)

# Linking
link_directories(
    "${sdl_root}/lib"
)

add_definitions(-DPANDORA)
 
# Location for Cannonball to create save files
# Used to auto-generate setup.hpp with various file paths
set(xml_directory home/)
set(sdl_flags "SDL_HWSURFACE")