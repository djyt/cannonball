cmake_minimum_required (VERSION 2.6.2)
project (outrun)

# Source and header locations
set(main_base ../src/main)
set(main_cpp_base ${main_base})

# Library locations
set(lib_base ../../lib)
set(boost_root ${lib_base}/boost_1_51_0)
set(sdl_root ${lib_base}/SDL-1.2.15)

# Include
include_directories(
    "${main_cpp_base}"
    "${boost_root}"    
)

# SDL includes
#TODO: This should not be platform specific, just need to agree standard
if (WIN32)
    include_directories(
        "${sdl_root}/include"
    )
else()
    include_directories( 
        "${sdl_root}/include/SDL"
    )
endif()

# Linking
link_directories(
    "${sdl_root}/lib"
)

# MINGW is fussy regarding library order.
# Specific order: mingw32, SDL, SDLmain
if (MINGW)
    link_libraries(outrun
    mingw32
    )
endif(MINGW)

link_libraries(outrun
    SDL
    SDLmain
)

set(src_main
    "${main_cpp_base}/menu.hpp"
    "${main_cpp_base}/globals.hpp"
    "${main_cpp_base}/romloader.hpp"
    "${main_cpp_base}/roms.hpp"
    "${main_cpp_base}/stdint.hpp"

    "${main_cpp_base}/main.cpp"
    "${main_cpp_base}/menu.cpp"
    "${main_cpp_base}/romloader.cpp"
    "${main_cpp_base}/roms.cpp"
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
    "${main_cpp_base}/sdl/audio.hpp"
    "${main_cpp_base}/sdl/timer.hpp"
    "${main_cpp_base}/sdl/input.hpp"
    "${main_cpp_base}/sdl/video.hpp"

    "${main_cpp_base}/sdl/audio.cpp"
    "${main_cpp_base}/sdl/timer.cpp"
    "${main_cpp_base}/sdl/input.cpp"
    "${main_cpp_base}/sdl/video.cpp"
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
    "${main_cpp_base}/engine/opalette.hpp"
    "${main_cpp_base}/engine/options.hpp"
    "${main_cpp_base}/engine/oroad.hpp"
    "${main_cpp_base}/engine/osmoke.hpp"
    "${main_cpp_base}/engine/osprite.hpp"
    "${main_cpp_base}/engine/osprites.hpp"
    "${main_cpp_base}/engine/ostats.hpp"
    "${main_cpp_base}/engine/otiles.hpp"
    "${main_cpp_base}/engine/otraffic.hpp"
    "${main_cpp_base}/engine/outils.hpp"
    "${main_cpp_base}/engine/outrun.hpp"
    "${main_cpp_base}/engine/ozoom_lookup.hpp"

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

set(src_engine_audio
    "${main_cpp_base}/engine/audio/commands.hpp"
    "${main_cpp_base}/engine/audio/osound.hpp"
    "${main_cpp_base}/engine/audio/osoundint.hpp"
    "${main_cpp_base}/engine/audio/osoundadr.hpp"

    "${main_cpp_base}/engine/audio/osound.cpp"
    "${main_cpp_base}/engine/audio/osoundint.cpp"
    )

# Group files into relevant filters for Visual Studio build
SOURCE_GROUP(src FILES ${src_main})
SOURCE_GROUP(src\\hwvideo FILES ${src_hwvideo})
SOURCE_GROUP(src\\hwaudio FILES ${src_hwaudio})
SOURCE_GROUP(src\\sdl FILES ${src_sdl})
SOURCE_GROUP(src\\engine FILES ${src_engine})
SOURCE_GROUP(src\\engine\\audio FILES ${src_engine_audio})

add_executable(outrun
    ${src_main}
    ${src_hwvideo}
    ${src_hwaudio}
    ${src_sdl}
    ${src_engine}
    ${src_engine_audio}
)

# Microsoft Visual Studio 2010
# Turn off some of the default libraries
# See: http://www.ogre3d.org/forums/viewtopic.php?f=10&t=69994
if (MSVC)
    MESSAGE ( STATUS "Turning default MSVC libraries off")
    SET_TARGET_PROPERTIES(outrun PROPERTIES 
    LINK_FLAGS "/NODEFAULTLIB:msvcrt.lib"
    )
endif(MSVC)

if (APPLE)
    FIND_LIBRARY(COCOA_LIBRARY Cocoa) 
    target_link_libraries(outrun
        ${COCOA_LIBRARY}
    )
endif(APPLE)

