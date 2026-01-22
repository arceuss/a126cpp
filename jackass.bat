@echo off
setlocal EnableExtensions

rem =========================
rem Paths
rem =========================
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "BUILD_DIR=%ROOT%\build_vs2015_win32_release"

rem SDL 1.2 (real) solution you already built successfully
set "SDL_SLN=%ROOT%\external\SDL-1.2\VisualC\SDL_VS2010.sln"

rem MSBuild (VS2010+ installs use this .NET MSBuild; works for your SDL sln per your log)
set "MSBUILD=%WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe"

rem CMake generator (VS2015)
set "CMAKE_GEN=Visual Studio 14 2015"

rem Optional XP targeting flags for YOUR project (only if your CMake supports it)
set "TARGET_XP=ON"

rem =========================
rem Sanity checks
rem =========================
if not exist "%ROOT%\CMakeLists.txt" (echo ERROR: CMakeLists.txt not found & exit /b 1)
if not exist "%SDL_SLN%" (echo ERROR: SDL sln not found: "%SDL_SLN%" & exit /b 1)
if not exist "%MSBUILD%" (echo ERROR: MSBuild not found: "%MSBUILD%" & exit /b 1)

where cmake >nul 2>&1
if errorlevel 1 (
  echo ERROR: cmake not found in PATH.
  exit /b 1
)

rem =========================
rem Build SDL 1.2 (Win32 Release)
rem =========================
echo === Building SDL 1.2 (Win32 Release) ===
"%MSBUILD%" "%SDL_SLN%" /m /t:Build /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 (
  echo ERROR: SDL 1.2 build failed.
  exit /b 1
)

rem SDL output paths (match your log)
set "SDL_INC=%ROOT%\external\SDL-1.2\include"
set "SDL_LIB=%ROOT%\external\SDL-1.2\VisualC\SDL\Win32\Release\SDL.lib"
set "SDL_DLL=%ROOT%\external\SDL-1.2\VisualC\SDL\Win32\Release\SDL.dll"
set "SDLMAIN_LIB=%ROOT%\external\SDL-1.2\VisualC\SDLmain\Win32\Release\SDLmain.lib"

if not exist "%SDL_INC%\SDL.h" (echo ERROR: Missing SDL.h in "%SDL_INC%" & exit /b 1)
if not exist "%SDL_LIB%" (echo ERROR: Missing "%SDL_LIB%" & exit /b 1)
if not exist "%SDL_DLL%" (echo ERROR: Missing "%SDL_DLL%" & exit /b 1)
if not exist "%SDLMAIN_LIB%" (echo WARNING: Missing "%SDLMAIN_LIB%" (continuing))

rem =========================
rem Configure + Build your project (VS2015 Win32 Release)
rem =========================
echo === Configuring project with CMake (%CMAKE_GEN% Win32) ===
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"
pushd "%BUILD_DIR%" >nul

cmake -G "%CMAKE_GEN%" ^
  -DSDL_INCLUDE_DIR:PATH="%SDL_INC%" ^
  -DSDL_LIBRARY:FILEPATH="%SDL_LIB%" ^
  -DSDLMAIN_LIBRARY:FILEPATH="%SDLMAIN_LIB%" ^
  -DALPHA126CPP_TARGET_WINXP:BOOL=%TARGET_XP% ^
  "%ROOT%"
if errorlevel 1 (
  echo ERROR: CMake configure failed.
  popd >nul
  exit /b 1
)

echo === Building project (Release) ===
cmake --build . --config Release
if errorlevel 1 (
  echo ERROR: Build failed.
  popd >nul
  exit /b 1
)

rem =========================
rem Copy SDL.dll next to built EXEs
rem =========================
echo === Copying SDL.dll next to executables ===
for /r "%BUILD_DIR%" %%F in (*.exe) do (
  copy /y "%SDL_DLL%" "%%~dpF" >nul
)

popd >nul
echo.
echo SUCCESS: Win32 Release build complete.
echo Build folder: "%BUILD_DIR%"
exit /b 0
