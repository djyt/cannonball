# -----------------------------------------------------------------------------
# CannonBall CMake setup
#
# For ports, please try not to edit this file directly.
# Everything should go into a win64.cmake equivalent.
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.8)
project(cannonball CXX)

add_executable(cannonball)

# -----------------------------------------------------------------------------
# Directory Setup
# -----------------------------------------------------------------------------

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Source location
set(main_cpp_base ${CMAKE_CURRENT_LIST_DIR}/src/main)

# Include
target_include_directories(cannonball PRIVATE
    ${main_cpp_base}
)

set(RENDER_ENGINE "OpenGL" CACHE STRING "Render engine library")
set_property(CACHE RENDER_ENGINE PROPERTY STRINGS OpenGL OpenGLES Software)

if(WIN32)
    set(HAVE_DIRECTX TRUE CACHE BOOL "Use DirectX (DInput8) for force feedback support")
endif()

# -----------------------------------------------------------------------------
# Find default packages
# -----------------------------------------------------------------------------

find_package(Boost REQUIRED)
find_package(SDL2 REQUIRED)

target_link_libraries(cannonball PRIVATE
    Boost::boost
    SDL2::SDL2
    SDL2::SDL2main
)

# -----------------------------------------------------------------------------
# Platform Specific Setup
# -----------------------------------------------------------------------------

# Include relevant CMAKE file (default, mingw, pandora etc.)
if(TARGET)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmake/${TARGET}")
        message(STATUS "CMake files for ${TARGET} found")
        include(cmake/${TARGET})
    else()
        message(WARNING "CMake files for ${TARGET} not found")
    endif()
endif()

# -----------------------------------------------------------------------------
# Project Source Files
# -----------------------------------------------------------------------------

set(src_main
    "${main_cpp_base}/globals.hpp"
    "${main_cpp_base}/romloader.hpp"
    "${main_cpp_base}/roms.hpp"
    "${main_cpp_base}/trackloader.hpp"
    "${main_cpp_base}/stdint.hpp"
    "${main_cpp_base}/main.hpp"
    "${main_cpp_base}/video.hpp"
    "${main_cpp_base}/utils.hpp"
    "${main_cpp_base}/windirent.h"

    "${main_cpp_base}/main.cpp"
    "${main_cpp_base}/romloader.cpp"
    "${main_cpp_base}/trackloader.cpp"
    "${main_cpp_base}/roms.cpp"
    "${main_cpp_base}/utils.cpp"
    "${main_cpp_base}/video.cpp"
)

set(src_frontend
    "${main_cpp_base}/frontend/cabdiag.hpp"
    "${main_cpp_base}/frontend/config.hpp"
    "${main_cpp_base}/frontend/menu.hpp"
    "${main_cpp_base}/frontend/menulabels.hpp"
    "${main_cpp_base}/frontend/ttrial.hpp"

    "${main_cpp_base}/frontend/cabdiag.cpp"
    "${main_cpp_base}/frontend/config.cpp"
    "${main_cpp_base}/frontend/menu.cpp"
    "${main_cpp_base}/frontend/ttrial.cpp"
)

set(src_hwvideo
    "${main_cpp_base}/hwvideo/hwroad.hpp"
    "${main_cpp_base}/hwvideo/hwsprites.hpp"
    "${main_cpp_base}/hwvideo/hwtiles.hpp"

    "${main_cpp_base}/hwvideo/hwroad.cpp"
    "${main_cpp_base}/hwvideo/hwsprites.cpp"
    "${main_cpp_base}/hwvideo/hwtiles.cpp"
)

set(src_hwaudio
    "${main_cpp_base}/hwaudio/segapcm.hpp"
    "${main_cpp_base}/hwaudio/soundchip.hpp"
    "${main_cpp_base}/hwaudio/ym2151.hpp"

    "${main_cpp_base}/hwaudio/segapcm.cpp"
    "${main_cpp_base}/hwaudio/soundchip.cpp"
    "${main_cpp_base}/hwaudio/ym2151.cpp"
)

set(src_sdl
    "${main_cpp_base}/sdl2/audio.hpp"
    "${main_cpp_base}/sdl2/input.hpp"
    "${main_cpp_base}/sdl2/renderbase.hpp"
    "${main_cpp_base}/sdl2/timer.hpp"

    "${main_cpp_base}/sdl2/audio.cpp"
    "${main_cpp_base}/sdl2/input.cpp"
    "${main_cpp_base}/sdl2/renderbase.cpp"
    "${main_cpp_base}/sdl2/timer.cpp"
)

if(RENDER_ENGINE STREQUAL "OpenGLES")
    set(src_render
        "${main_cpp_base}/sdl2/rendergles.hpp"
        "${main_cpp_base}/sdl2/rendergles.cpp"
    )
elseif(RENDER_ENGINE STREQUAL "OpenGL")
    set(src_render
        "${main_cpp_base}/sdl2/rendergl.hpp"
        "${main_cpp_base}/sdl2/rendergl.cpp"
    )
else()
    set(src_render
        "${main_cpp_base}/sdl2/rendersurface.hpp"
        "${main_cpp_base}/sdl2/rendersurface.cpp"
    )
endif()

set(src_directx
    "${main_cpp_base}/directx/ffeedback.hpp"
    "${main_cpp_base}/directx/ffeedback.cpp"
)

set(src_engine
    "${main_cpp_base}/engine/oaddresses.hpp"
    "${main_cpp_base}/engine/oanimseq.hpp"
    "${main_cpp_base}/engine/oanimsprite.hpp"
    "${main_cpp_base}/engine/oattractai.hpp"
    "${main_cpp_base}/engine/obonus.hpp"
    "${main_cpp_base}/engine/ocrash.hpp"
    "${main_cpp_base}/engine/oentry.hpp"
    "${main_cpp_base}/engine/oferrari.hpp"
    "${main_cpp_base}/engine/ohiscore.hpp"
    "${main_cpp_base}/engine/ohud.hpp"
    "${main_cpp_base}/engine/oinitengine.hpp"
    "${main_cpp_base}/engine/oinputs.hpp"
    "${main_cpp_base}/engine/olevelobjs.hpp"
    "${main_cpp_base}/engine/ologo.hpp"
    "${main_cpp_base}/engine/omap.hpp"
    "${main_cpp_base}/engine/omusic.hpp"
    "${main_cpp_base}/engine/ooutputs.hpp"
    "${main_cpp_base}/engine/opalette.hpp"
    "${main_cpp_base}/engine/oroad.hpp"
    "${main_cpp_base}/engine/osmoke.hpp"
    "${main_cpp_base}/engine/osprite.hpp"
    "${main_cpp_base}/engine/osprites.hpp"
    "${main_cpp_base}/engine/ostats.hpp"
    "${main_cpp_base}/engine/otiles.hpp"
    "${main_cpp_base}/engine/otraffic.hpp"
    "${main_cpp_base}/engine/outils.hpp"
    "${main_cpp_base}/engine/outrun.hpp"

    "${main_cpp_base}/engine/oanimseq.cpp"
    "${main_cpp_base}/engine/oattractai.cpp"
    "${main_cpp_base}/engine/obonus.cpp"
    "${main_cpp_base}/engine/ocrash.cpp"
    "${main_cpp_base}/engine/oferrari.cpp"
    "${main_cpp_base}/engine/ohiscore.cpp"
    "${main_cpp_base}/engine/ohud.cpp"
    "${main_cpp_base}/engine/oinitengine.cpp"
    "${main_cpp_base}/engine/oinputs.cpp"
    "${main_cpp_base}/engine/olevelobjs.cpp"
    "${main_cpp_base}/engine/ologo.cpp"
    "${main_cpp_base}/engine/omap.cpp"
    "${main_cpp_base}/engine/omusic.cpp"
    "${main_cpp_base}/engine/ooutputs.cpp"
    "${main_cpp_base}/engine/opalette.cpp"
    "${main_cpp_base}/engine/oroad.cpp"
    "${main_cpp_base}/engine/osmoke.cpp"
    "${main_cpp_base}/engine/osprite.cpp"
    "${main_cpp_base}/engine/osprites.cpp"
    "${main_cpp_base}/engine/ostats.cpp"
    "${main_cpp_base}/engine/otiles.cpp"
    "${main_cpp_base}/engine/otraffic.cpp"
    "${main_cpp_base}/engine/outils.cpp"
    "${main_cpp_base}/engine/outrun.cpp"
)

set(src_engine_data
    "${main_cpp_base}/engine/data/ozoom_lookup.hpp"
    "${main_cpp_base}/engine/data/sprite_pals.hpp"
)

set(src_engine_audio
    "${main_cpp_base}/engine/audio/commands.hpp"
    "${main_cpp_base}/engine/audio/osound.hpp"
    "${main_cpp_base}/engine/audio/osoundadr.hpp"
    "${main_cpp_base}/engine/audio/osoundint.hpp"

    "${main_cpp_base}/engine/audio/osound.cpp"
    "${main_cpp_base}/engine/audio/osoundint.cpp"
)

set(SRCS
    ${src_main}
    ${src_frontend}
    ${src_hwvideo}
    ${src_hwaudio}
    ${src_sdl}
    ${src_render}
    ${src_directx}
    ${src_engine}
    ${src_engine_data}
    ${src_engine_audio}
)

# Group files into relevant filters for Visual Studio build
source_group(src FILES ${src_main})
source_group(src\\frontend FILES ${src_frontend})
source_group(src\\hwvideo FILES ${src_hwvideo})
source_group(src\\hwaudio FILES ${src_hwaudio})
source_group(src\\sdl FILES ${src_sdl})
source_group(src\\sdl FILES ${src_render})
source_group(src\\directx FILES ${src_directx})
source_group(src\\engine FILES ${src_engine})
source_group(src\\engine\\data FILES ${src_engine_data})
source_group(src\\engine\\audio FILES ${src_engine_audio})

# Add Icon For Windows Builds
if(WIN32)
    set(SRCS ${SRCS} ../res/cannonball.rc)
endif()

# -----------------------------------------------------------------------------
# Library Setup
# -----------------------------------------------------------------------------

target_sources(cannonball PRIVATE ${SRCS})

target_link_directories(cannonball PRIVATE
    ${platform_link_dirs}
)

target_link_libraries(cannonball PRIVATE
    ${platform_link_libs}
)

if(WIN32)
    if(HAVE_DIRECTX)
        add_definitions(-DHAVE_DIRECTX)

        target_link_libraries(cannonball PRIVATE
            dxguid   # Direct X Haptic Support
            dinput8  # Direct X Haptic Support
        )
    endif()
else()
    set(OpenGL_GL_PREFERENCE "GLVND" CACHE STRING "OpenGL driver preference")
endif()

if(RENDER_ENGINE STREQUAL "OpenGLES")
    message(STATUS "Use OpenGL ES backend")
    find_package(OpenGLES REQUIRED)
    target_link_libraries(cannonball PRIVATE ${OPENGLES_LIBRARIES})
    target_compile_definitions(cannonball PRIVATE WITH_OPENGLES)
elseif(RENDER_ENGINE STREQUAL "OpenGL")
    message(STATUS "Use OpenGL backend")
    find_package(OpenGL REQUIRED)
    target_link_libraries(cannonball PRIVATE OpenGL::GL OpenGL::GLU)
    target_compile_definitions(cannonball PRIVATE WITH_OPENGL)
endif()

# -----------------------------------------------------------------------------
# Copy useful files to build destination
# -----------------------------------------------------------------------------

configure_file(res/config.xml ${CMAKE_CURRENT_BINARY_DIR}/config.xml COPYONLY)

configure_file(res/tilemap.bin ${CMAKE_CURRENT_BINARY_DIR}/res/tilemap.bin COPYONLY)

configure_file(res/tilepatch.bin ${CMAKE_CURRENT_BINARY_DIR}/res/tilepatch.bin COPYONLY)

configure_file(res/gamecontrollerdb.txt ${CMAKE_CURRENT_BINARY_DIR}/res/gamecontrollerdb.txt COPYONLY)
