@echo off

set WORKING_DIR="C:\coding\cannonball\emscripten"
set FIREFOX_DIR="C:\Program Files (x86)\FirefoxNightly"

if "%1"=="cmake" goto cmake
if "%1"=="build" goto build
if "%1"=="convertold" goto convertold
if "%1"=="convertslow" goto convertslow
if "%1"=="convert" goto convert
if "%1"=="run" goto run
if "%1"=="clean" goto clean

:cmake
if not exist %WORKING_DIR% mkdir %WORKING_DIR%
cd %WORKING_DIR%
cmake -DEMSCRIPTEN=1 -DCMAKE_TOOLCHAIN_FILE=c:/utils/emscripten/emscripten/cmake/Platform/Emscripten.cmake -DCMAKE_MODULE_PATH=c:/utils/emscripten/emscripten/cmake -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles" -DTARGET=emscripten ../cmake
cd..
goto end

:build
cd %WORKING_DIR%
mingw32-make
cd..
goto end

:convertold
cd %WORKING_DIR%
echo Converting .bc to .js (OPTIMIZED)
call emcc -O2 --llvm-lto 1 -closure 1 -s DOUBLE_MODE=0 cannonball.bc -o cannonball.html --preload-file C:\coding\cannonball\roms@roms
cd..
goto end

:convertslow
cd %WORKING_DIR%
echo Converting .bc to .js (Not Optimized)
call emcc -s EXPORTED_FUNCTIONS="['_emscripten_init', '_emscripten_tick', '_emscripten_setfps', '_emscripten_audio', '_emscripten_set_framesize', '_emscripten_loadzip']" ^
cannonball.bc -o cannonball.html --preload-file C:\coding\cannonball\roms@roms

cd..
goto end

:convert
cd %WORKING_DIR%
echo Converting with new html... (OPTIMIZED)

rem call emcc -O2 --llvm-lto 1 -closure 1 -s DOUBLE_MODE=0 ^
call emcc -O2 -s DOUBLE_MODE=0 -closure 1 ^
-s EXPORTED_FUNCTIONS="['_emscripten_init', '_emscripten_tick', '_emscripten_setfps', '_emscripten_audio', '_emscripten_set_framesize', '_emscripten_loadzip']" ^
cannonball.bc ../libz.bc -o cannonball.html
rem --preload-file C:\coding\cannonball\roms@roms

echo Extracting Javascript from HTML
cd..
python extract_script.py %WORKING_DIR%/cannonball.html > %WORKING_DIR%/cannonball.html.js
del %WORKING_DIR%\cannonball.html
copy cannonball.html %WORKING_DIR%
goto end

:run
cd %WORKING_DIR%
%FIREFOX_DIR%\firefox %WORKING_DIR%\cannonball.html
cd..
goto end

:clean
if not exist %WORKING_DIR% echo Directory: %WORKING_DIR% not found!
if exist %WORKING_DIR% rm -rf emscripten
goto end

:end