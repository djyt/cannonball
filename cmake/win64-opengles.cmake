# -----------------------------------------------------------------------------
# CannonBall Windows Setup
#
# Extra libraries are required to use OpenGLES on Windows. 
# Quite a long install process.
# Download & Compile Angle: https://opensource.google/projects/angle
#
# Other notes:
# http://mickcharlesbeaver.blogspot.com/2017/11/using-opengl-es-20-with-sdl2-via-angle.html
# -----------------------------------------------------------------------------

# Library Locations
set(lib_base c:/coding/lib)
set(boost_dir ${lib_base}/boost_1_74_0)
set(sdl2_dir ${lib_base}/SDL2-2.0.12)
set(angle_dir ${lib_base}/angle/out/Debug)
set(dx8_dir c:/dxsdk)

# Use OpenGLES for rendering.
set(OPENGLES 1)

# Platform Specific Libraries
set(platform_link_libs
    "${angle_dir}/libEGL.dll.lib"
    "${angle_dir}/libGLESv2.dll.lib"
    dxguid   # Direct X Haptic Support
    dinput8  # Direct X Haptic Support
)

# Platform Specific Link Directories
set(platform_link_dirs
    "${dx8_dir}/lib"
)