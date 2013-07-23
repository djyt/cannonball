# Emscripten Javascript Build
add_definitions(-DEMSCRIPTEN)

# Use OpenGL for rendering.
set(OPENGL 0)

# Supress useless compiler warnings
set(CMAKE_CXX_FLAGS "-Qunused-arguments")

# XML Config not used by Emscripten
set(xml_directory ./)

# SDL Software Rendering Flags (ignored if OpenGL used)
set(sdl_flags "SDL_SWSURFACE | SDL_DOUBLEBUF")