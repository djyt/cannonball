# MinGW Compiler
# SDL Setup is different

set(lib_base c:/coding/lib)
set(sdl_root ${lib_base}/SDL-1.2.15)
set(SDL_INCLUDE_DIR ${lib_base}/SDL-1.2.15/include)

find_package(SDL REQUIRED)

include_directories("${SDL_INCLUDE_DIR}")

link_libraries(cannonball 
    ${SDL_LIBRARY}
    SDLmain
)

set(CMAKE_CXX_FLAGS "-Ofast")
 
# Location for Cannonball to create save files
# Used to auto-generate setup.hpp with various file paths
set(xml_directory ./)
set(sdl_flags "SDL_DOUBLEBUF | SDL_SWSURFACE")