# Default CMake Setup. Used for Mac OS x builds
EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
EXECUTE_PROCESS( COMMAND xcode-select -p COMMAND tr -d '\n' OUTPUT_VARIABLE XCODE_DEV_PATH )
message( STATUS "Architecture: ${ARCHITECTURE}" )
message( STATUS "Using Xcode from: ${XCODE_DEV_PATH}" )

# For INTeL 32bit Mac OS X 10.5+
set(CMAKE_CXX_FLAGS " -O2 -force_cpusubtype_ALL -arch ${ARCHITECTURE} -m32 -isysroot ${XCODE_DEV_PATH}/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -mmacosx-version-min=10.7")

# # For PowerPC 32bit Mac OS X 10.5+
# set(CMAKE_CXX_FLAGS " -std=c++11 -stdlib=libstdc++ -O2 -faltivec -force_cpusubtype_ALL -mmacosx-version-min=10.5 -arch ppc -m32 -isysroot ${XCODE_DEV_PATH}/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk -mmacosx-version-min=10.7")

# Use OpenGL for rendering
set(OPENGL 1)

find_package(SDL REQUIRED)
find_package(Boost REQUIRED)

include_directories(${SDL_INCLUDE_DIR} "${lib_base}/include/" "~/Library/Frameworks/SDL.framework/Headers" "${XCODE_DEV_PATH}/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/X11/include/" ${Boost_INCLUDE_DIRS})

find_library(COCOA_LIBRARY Cocoa)
find_library(GLUT_LIBRARY GLUT )
find_library(OpenGL_LIBRARY OpenGL )

link_libraries(cannonball 
    ${SDL_LIBRARY}
    ${COCOA_LIBRARY}
    ${GLUT_LIBRARY}
    ${OpenGL_LIBRARY}
    ${Boost_LIBRARIES}
)

# Linking
link_directories(
    "${sdl_root}/lib"
)
 
# Location for Cannonball to create save files
# Used to auto-generate setup.hpp with various file paths
set(xml_directory ./)
set(sdl_flags "SDL_DOUBLEBUF | SDL_SWSURFACE")
set(sdl_src_cocoa
	"${main_cpp_base}/sdl/SDLMain.h"
	"${main_cpp_base}/sdl/SDLMain.m"
)
set(CMAKE_CXX_FLAGS "-stdlib=libc++")
