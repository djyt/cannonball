# Default CMake Setup. Used for Mac OS x builds
EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message( STATUS "Architecture: ${ARCHITECTURE}" )

# For INTeL 32bit Mac OS X 10.5+
set(CMAKE_CXX_FLAGS " -O2 -force_cpusubtype_ALL -arch ${ARCHITECTURE} -m32 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -mmacosx-version-min=10.7")

# # For PowerPC 32bit Mac OS X 10.5+
# set(CMAKE_CXX_FLAGS " -std=c++11 -stdlib=libstdc++ -O2 -faltivec -force_cpusubtype_ALL -mmacosx-version-min=10.5 -arch ppc -m32 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk -mmacosx-version-min=10.7")

# Use OpenGL for rendering
set(OPENGL 1)

find_package(SDL REQUIRED)
find_package(Boost REQUIRED)

include_directories(${SDL_INCLUDE_DIR} "${lib_base}/include/" "~/Library/Frameworks/SDL.framework/Headers" "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/X11/include/" ${Boost_INCLUDE_DIRS})

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
