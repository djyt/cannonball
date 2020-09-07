Cannonball - OutRun Engine
==========================

See [Reassembler Blog](http://reassembler.blogspot.co.uk/).

Credits
-------

* Chris White - Project creator.


Getting Started
---------------

CannonBall is coded in C++ and makes use of the SDL 2 and Boost libraries. 
CannonBall has been successfully built for Windows, Mac OS X, Linux, Open Pandora and the Raspberry Pi.
CannonBall can use OpenGL, OpenGLES (mobile platforms) or plain SDL2 for rendering. 

I have recently deprecated support for SDL 1, to focus on SDL 2. But feel free to grab an older version from github if you need it. 

* Install your favourite C++ environment (e.g. GCC, Visual Studio, Xcode, MingGW etc.)
* Install [CMake](http://www.cmake.org/). This generates your platform and compiler build files. 
* Extract the [Boost Library](http://www.boost.org/) somewhere, for example: c:\coding\lib\boost_1_74_0
* Extract the [SDL Development Library](https://www.libsdl.org/download-2.0.php) somewhere, for example: c:\coding\lib\SDL2-2.0.12
* Read the SDL documentation & build the SDL Library for your system.
* Windows only: I needed to copy cannonball/cmake/windows_copy_to_sdl2_lib_directory/sdl2-config.cmake to c:\coding\lib\SDL2-2.0.12
* Windows only: download and install the [Direct 8.1 SDK](https://archive.org/details/dx81sdk_full)
* Extract the Cannonball code somewhere, for example: c:\coding\cannonball
* You may need to create a .cmake file for your system to configure specific options. See the cmake subdirectory for more details.

Build
-----

* Run CMake to generate the relevant build files for your compiler. You can optionally pass -DTARGET=filename to pass a custom .cmake file
* Compile using your chosen compiler. Further details below.

### Visual Studio 2019 Community Edition

    cmake -G ../cmake

* Open the created CannonBall solution in VS 2019. 
* Right click and choose 'Set as StartUp project'. 
* Set working directory to something sensible. Right click -> Configuration properties -> Debugging
* Copy SDL2.DLL and the roms subdirectory to the working directory.
* Copy the OutRun revision B romset to the roms subdirectory. Rename the files if necessary.
* You can then compile, debug and run from Visual Studio as expected.
