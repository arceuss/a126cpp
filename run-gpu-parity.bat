@echo off
rem Rebuilds the native and gl33 GPU parity fixtures in build-parity and runs
rem record + compare, so a backend change is checked pixel-for-pixel.
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
set VSCMAKE=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
cd /d C:\Users\arceus\a126cpp-portable\a126cpp
"%VSCMAKE%" --build build-parity --target a126cpp-legacygl-gpu-parity-gl33 a126cpp-legacygl-gpu-parity-native
if errorlevel 1 exit /b 1
cd build-parity
ctest -R "legacygl-gpu-(record-native|record-gl33|compare)$" --output-on-failure
exit /b %errorlevel%
