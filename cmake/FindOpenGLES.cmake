# Finds the OpenGLES 2 and EGL libraries
#
# This will define the following variables:
#
# OPENGLES_FOUND - system has OpenGLES
# OPENGLES_INCLUDE_DIRS - the OpenGLES include directory
# OPENGLES_LIBRARIES - the OpenGLES and EGL libraries
# OPENGLES_DEFINITIONS - the OpenGLES definitions

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_OPENGLES glesv2 QUIET)
endif()

if(WIN32)
     if(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE STREQUAL "x86")
        set(ANGLE_LIB_PATH_GUESS bin/UAP/Win32)
    else()
        set(ANGLE_LIB_PATH_GUESS bin/UAP/${CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE})
    endif()
endif()

if(NOT CORE_SYSTEM_NAME STREQUAL darwin_embedded)
    find_path(OPENGLES_INCLUDE_DIR
        GLES2/gl2.h
        PATHS ${PC_OPENGLES_INCLUDEDIR}
            "$ENV{ANGLE_DIR}/Include/"
            "${ANGLE_DIR}/Include/")
    find_library(OPENGLES_gl_LIBRARY
        NAMES GLESv2 libGLESv2
        PATHS ${PC_OPENGLES_LIBDIR}
            "$ENV{ANGLE_DIR}/${ANGLE_LIB_PATH_GUESS}"
            "${ANGLE_DIR}/${ANGLE_LIB_PATH_GUESS}")
    find_library(OPENGLES_egl_LIBRARY
        NAMES EGL libEGL
        PATHS ${PC_OPENGLES_LIBDIR}
            "$ENV{ANGLE_DIR}/${ANGLE_LIB_PATH_GUESS}"
            "${ANGLE_DIR}/${ANGLE_LIB_PATH_GUESS}")
else()
    find_library(OPENGLES_gl_LIBRARY
        NAMES OpenGLES
        PATHS ${CMAKE_OSX_SYSROOT}/System/Library
        PATH_SUFFIXES Frameworks
        NO_DEFAULT_PATH)
    find_library(OPENGLES_egl_LIBRARY
        NAMES EGL
        PATHS ${CMAKE_OSX_SYSROOT}/System/Library
        PATH_SUFFIXES Frameworks
        NO_DEFAULT_PATH)
    set(OPENGLES_INCLUDE_DIR ${OPENGLES_gl_LIBRARY}/Headers)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenGLES
    REQUIRED_VARS OPENGLES_gl_LIBRARY OPENGLES_egl_LIBRARY OPENGLES_INCLUDE_DIR)

mark_as_advanced(OPENGLES_INCLUDE_DIR OPENGLES_gl_LIBRARY OPENGLES_egl_LIBRARY)

set(OPENGLES_LIBRARIES ${OPENGLES_gl_LIBRARY} ${OPENGLES_egl_LIBRARY})
set(OPENGLES_INCLUDE_DIRS ${OPENGLES_INCLUDE_DIR})
set(OPENGLES_DEFINITIONS -DHAS_GLES=2)


