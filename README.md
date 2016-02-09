Cannonball - OutRun Engine
==========================

See [Reassembler Blog](http://reassembler.blogspot.co.uk/).

Credits
-------

* Chris White - Project creator.
* Arun Horne  - Cross platform work.

Getting Started
---------------

Cannonball has been successfully built for Windows, Mac OS X, Linux, Open Pandora and the Raspberry Pi. 

* Install your favourite C++ environment (e.g. GCC, Visual Studio, Xcode, MingGW etc.)
* Install [CMake](http://www.cmake.org/). This generates your platform and compiler build files. 
* Extract the [Boost Library](http://www.boost.org/) somewhere, for example: c:\coding\lib\boost_1_51_0
* Extract the [SDL Library](http://www.libsdl.org/download-1.2.php) somewhere, for example: c:\coding\lib\SDL-1.2.15
* Read the SDL documentation & compile the SDL Library for your system.
* Windows only, download and install the [Direct 8.1 SDK](http://stackoverflow.com/questions/5192384/looking-for-the-old-directx-8-1-sdk)
* Extract the Cannonball code somewhere, for example: c:\coding\cannonball
* You may need to create a .cmake file for your system to configure specific options. See the cmake subdirectory for more details. If not, the default.cmake file will be used.

Build
-----

* Run CMake to generate the relevant build files for your compiler. You can optionally pass -DTARGET=filename to pass a custom .cmake file
* Compile using your chosen compiler. Further details below.

###

    mkdir build
    cd build

### Non-IDE (e.g. straight GCC)
    
    cmake -G "Insert Generator Name Here" ../cmake
    make

### MinGW

    cmake -G "MinGW Makefiles" -DTARGET=mingw ../cmake
    mingw32-make
    
### Visual Studio 2010

    cmake -G "Visual Studio 10" ../cmake

### Mac OSX

    cmake -G "Unix Makefiles" -DTARGET:STRING=macosx ../cmake
    make

* Copy SDL.DLL and the roms subdirectory to the build directory.
* Right click the 'cannonball' project in the IDE and choose 'Set as StartUp project'. 
* You can then compile, debug and run from Visual Studio as expected.

Run
---

* Copy the OutRun revision B romset to the roms subdirectory. Rename the files if necessary.
* Copy or link the roms subdirectory to whereever your executable resides.

###

    ln -s ../roms roms
    ./outrun
    
    
Building SDL-1.2.15
-------------------

### Darwin

    ./configure --prefix=~/SDL-1.2.15/build --disable-assembly

### MinGW

See: [Setting up MSYS MinGW build system for compiling SDL OpenGL applications](http://blog.pantokrator.net/2006/08/08/setting-up-msysmingw-build-system-for-compiling-sdlopengl-applications/).

Execute the below commands from the msys environment.
    
    ./configure --prefix=/mingw --enable-stdio-redirect=no
    make
    make install
