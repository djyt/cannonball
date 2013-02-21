# MinGW Compiler
# SDL Setup is different

set(lib_base c:/coding/lib)
set(sdl_root ${lib_base}/SDL-1.2.15)
set(SDL_INCLUDE_DIR ${lib_base}/SDL-1.2.15/include)

if (NOT DEFINED ENV{DXSDK})
    message(FATAL_ERROR "Warning: DirectX SDK Variable DXSDK Not Defined!")
endif()

find_package(SDL REQUIRED)

include_directories(
    "${SDL_INCLUDE_DIR}"
    # You may need to remove the next line based on your DirectX setup
    "$ENV{DXSDK}/include"
)

link_libraries(cannonball 
    ${SDL_LIBRARY}
    SDLmain
    dxguid
    dinput8
)

set(CMAKE_CXX_FLAGS "-Ofast -static-libgcc -static-libstdc++")
 
# Location for Cannonball to create save files
# Used to auto-generate setup.hpp with various file paths
set(xml_directory ./)
set(sdl_flags "SDL_DOUBLEBUF | SDL_SWSURFACE")