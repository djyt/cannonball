Cannonball - OutRun Engine
==========================

See [Reassembler Blog](http://reassembler.blogspot.co.uk/).

Credits
-------

* Chris White - Project creator.
* Arun Horne  - Cross platform work.

Getting Started
---------------
* Install your favourite C++ environment (e.g. GCC, Visual Studio, Xcode, MingGW etc.)
* Install [CMake](http://www.cmake.org/). This generates your platform and compiler build files. 
* Extract the [Boost Library](http://www.boost.org/) somewhere, for example: c:\dev\lib\boost_1_51_0
* Extract the [SDL Library](http://www.libsdl.org/download-1.2.php) somewhere, for example: c:\dev\lib\SDL-1.2.15
* Read the SDL documentation & compile the SDL Library for your system.
* Extract the Cannonball code somewhere, for example: c:\dev\cannonball
* Configure the CMakeLists.txt file to reflect the correct directory structure for your libraries.

Build
-----

* Run CMake to generate the relevant build files for your compiler.
* Compile using your chosen compiler. Further details below.

###

    mkdir build
    cd build

### Non-IDE (e.g. straight GCC)
    
    cmake -G "Insert Generator Name Here" ../cmake
    make

### MingGW

    cmake -G "MinGW Makefiles" ../cmake
    mingw32-make
    
### Visual Studio

* Copy SDL.DLL and the roms subdirectory to the build directory.
* Right click the 'outrun' project in the IDE and choose 'Set as StartUp project'. 
* You can then compile, debug and run from Visual Studio as expected.

Run
---

* Copy the OutRun revision B romset to the roms subdirectory. Rename the files if necessary.
* Copy or link the roms subdirectory to whereever your executable resides.

###

    ln -s ../roms roms
    ./outrun
    
    
Building SDL-1.2.15 on Darwin
-----------------------------

    ./configure --prefix=~/SDL-1.2.15/build --disable-assembly

