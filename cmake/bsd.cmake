# Default CMake Setup. Used for BSD (FreeBSD/NetBSD) Builds.

set(lib_base ${BSD_PREFIX_PATH}/include)

set(BOOST_INCLUDEDIR ${lib_base})

# Use legacy SDL1.x if requested
if(SDL)
        set(sdl_root ${lib_base}/SDL)

        link_libraries(cannonball
            SDL
        )

        set(sdl_flags "SDL_DOUBLEBUF | SDL_SWSURFACE")
else()  # Use SDL2 by default
        set(sdl_root ${lib_base}/SDL2)

        link_libraries(cannonball
            SDL2
        )

        add_definitions(-O3 -DSDL2)

        set(sdl_flags "SDL_WINDOW_RESIZABLE")

        # Set SDL2 instead of SDL1
        set(SDL2 1)
endif()

include_directories("${sdl_root}")

# Linking
link_directories(
    "${BSD_PREFIX_PATH}/lib"
)

# Location for Cannonball to create save files
# Used to auto-generate setup.hpp with various file paths
if(NOT DEFINED xml_directory)
        set(xml_directory ./)
endif()

