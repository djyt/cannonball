# Default CMake Setup. Used for BSD (FreeBSD/NetBSD) Builds.

set(lib_base ${BSD_PREFIX_PATH}/include)

set(BOOST_INCLUDEDIR ${lib_base})

set(sdl_root ${lib_base}/SDL)

include_directories("${sdl_root}")

link_libraries(cannonball
    SDL
)

# Linking
link_directories(
    "${BSD_PREFIX_PATH}/lib"
)

# Location for Cannonball to create save files
# Used to auto-generate setup.hpp with various file paths
set(xml_directory ./)
set(sdl_flags "SDL_DOUBLEBUF | SDL_SWSURFACE")
