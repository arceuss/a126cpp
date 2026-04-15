# UWP toolchain: targets Windows Store (UWP)
set(CMAKE_SYSTEM_NAME WindowsStore)
set(CMAKE_SYSTEM_VERSION 10.0)
# Do not let Visual Studio/CMake inherit the newest installed SDK as MinVersion;
# the package manifest targets Windows 10 1809+ (10.0.17763.0).
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION 10.0.17763.0)
