Outrun Port
===========

Credits
-------

It's all down to Chris; See [Reassembler Blog](http://reassembler.blogspot.co.uk/).

Build & Run
-----------

### Non-IDE

    mdkir build && cd build
    cmake ../cmake
    make
    ln -s ../roms roms
    ./outrun

### Xcode

Before can run, will need to link roms directory:

    cd xcode/Debug 
    ln -s ../../roms roms

### Visual Studio

TBC - but will need to make roms directory available

Building SDL-1.2.15 on Darwin
-----------------------------

    ./configure --prefix=~/SDL-1.2.15/build --disable-assembly

