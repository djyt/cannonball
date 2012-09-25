cmake_minimum_required (VERSION 2.6.2)
project (outrun)

#Source and header locations
set(main_base ../src/main)
set(main_cpp_base ${main_base}/cpp)
set(main_hpp_base ${main_base}/hpp)

#Dependency locations
set(boost_root c:/coding/lib/boost_1_51_0)
set(sdl_root c:/coding/lib/SDL-1.2.14)

#Build the program!
include_directories(
    "${main_hpp_base}"
    "${boost_root}"    
    "${sdl_root}/include"
)

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

add_executable(outrun

    #Source
    "${main_cpp_base}/asmhelper.cpp"
    "${main_cpp_base}/hwroad.cpp"
    "${main_cpp_base}/hwsprites.cpp"
    "${main_cpp_base}/hwtiles.cpp"
    "${main_cpp_base}/input.cpp"
    "${main_cpp_base}/main.cpp"
    "${main_cpp_base}/menu.cpp"
    "${main_cpp_base}/oanimseq.cpp"
    "${main_cpp_base}/oattractai.cpp"
    "${main_cpp_base}/obonus.cpp"
    "${main_cpp_base}/ocrash.cpp"
    "${main_cpp_base}/oferrari.cpp"
    "${main_cpp_base}/ohiscore.cpp"
    "${main_cpp_base}/ohud.cpp"
    "${main_cpp_base}/oinitengine.cpp"
    "${main_cpp_base}/oinputs.cpp"
    "${main_cpp_base}/olevelobjs.cpp"
    "${main_cpp_base}/ologo.cpp"
    "${main_cpp_base}/omap.cpp"
    "${main_cpp_base}/omusic.cpp"
    "${main_cpp_base}/opalette.cpp"
    "${main_cpp_base}/oroad.cpp"
    "${main_cpp_base}/osmoke.cpp"
    "${main_cpp_base}/osprite.cpp"
    "${main_cpp_base}/osprites.cpp"
    "${main_cpp_base}/ostats.cpp"
    "${main_cpp_base}/otiles.cpp"
    "${main_cpp_base}/otraffic.cpp"
    "${main_cpp_base}/outils.cpp"
    "${main_cpp_base}/outrun.cpp"
    "${main_cpp_base}/romloader.cpp"
    "${main_cpp_base}/roms.cpp"
    "${main_cpp_base}/timer.cpp"
    "${main_cpp_base}/video.cpp"

    #Header (included so auto-included in IDE projects)
    "${main_hpp_base}/Timer.hpp"
    "${main_hpp_base}/asmhelper.hpp"
    "${main_hpp_base}/controls.hpp"
    "${main_hpp_base}/hwroad.hpp"
    "${main_hpp_base}/hwsprites.hpp"
    "${main_hpp_base}/hwtiles.hpp"
    "${main_hpp_base}/input.hpp"
    "${main_hpp_base}/menu.hpp"
    "${main_hpp_base}/oaddresses.hpp"
    "${main_hpp_base}/oanimseq.hpp"
    "${main_hpp_base}/oanimsprite.hpp"
    "${main_hpp_base}/oattractai.hpp"
    "${main_hpp_base}/obonus.hpp"
    "${main_hpp_base}/ocrash.hpp"
    "${main_hpp_base}/oentry.hpp"
    "${main_hpp_base}/oferrari.hpp"
    "${main_hpp_base}/oglobals.hpp"
    "${main_hpp_base}/ohiscore.hpp"
    "${main_hpp_base}/ohud.hpp"
    "${main_hpp_base}/oinitengine.hpp"
    "${main_hpp_base}/oinputs.hpp"
    "${main_hpp_base}/ojumpentry.hpp"
    "${main_hpp_base}/olevelobjs.hpp"
    "${main_hpp_base}/ologo.hpp"
    "${main_hpp_base}/omap.hpp"
    "${main_hpp_base}/omusic.hpp"
    "${main_hpp_base}/opalette.hpp"
    "${main_hpp_base}/options.hpp"
    "${main_hpp_base}/oroad.hpp"
    "${main_hpp_base}/osmoke.hpp"
    "${main_hpp_base}/osprite.hpp"
    "${main_hpp_base}/osprites.hpp"
    "${main_hpp_base}/ostats.hpp"
    "${main_hpp_base}/otiles.hpp"
    "${main_hpp_base}/outils.hpp"
    "${main_hpp_base}/outrun.hpp"
    "${main_hpp_base}/ozoom_lookup.hpp"
    "${main_hpp_base}/romloader.hpp"
    "${main_hpp_base}/roms.hpp"
    "${main_hpp_base}/stdint.hpp"
    "${main_hpp_base}/video.hpp"
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

