# -----------------------------------------------------------------------------
# CannonBall Linux Setup
# -----------------------------------------------------------------------------

# Use OpenGL for rendering.
find_package(OpenGL REQUIRED)

# Platform Specific Libraries
set(platform_link_libs
    ${OPENGL_LIBRARIES}
)