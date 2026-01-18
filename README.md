# a126cpp

c++ port of minecraft alpha 1.2.6. was a test to see if cursor could port alphaplace client to c++17

<img width="856" height="512" alt="image" src="https://github.com/user-attachments/assets/8b0263b7-a6db-4b3f-92a5-4172eb8ee720" />

## building

you need cmake 3.15+ and a c++ compiler. on windows you can use visual studio or just gcc if you have it. all the dependencies are in the `external/` folder so you dont need to install anything else.

### Linux/Ubuntu dependencies

on linux/ubuntu, you need to install some system packages:

```bash
sudo apt update
sudo apt install -y build-essential cmake git pkg-config \
    libgl1-mesa-dev libegl1-mesa-dev \
    libasound2-dev libpulse-dev \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev \
    libwayland-dev libxkbcommon-dev libdrm-dev libgbm-dev \
    libdbus-1-dev libudev-dev
```

or run the install script:
```bash
chmod +x install-ubuntu-deps.sh
./install-ubuntu-deps.sh
```

### building

to build:
1. do git clone --recurse-submodules https://github.com/arceuss/a126cpp.git
2. make a build directory: `mkdir build && cd build`
3. run cmake: `cmake ..` (or `cmake -G "Visual Studio 17 2022" ..` for visual studio)
4. build it: `cmake --build .` (or just open the .sln in visual studio)

the executable will be in `bin/Alpha126Cpp.exe` (or just `Alpha126Cpp` on linux).

## resources

already included so you can be lazy

## dependencies

everything is included as submodules or in the external folder:
- SDL2 for windowing and input
- zlib for compression
- stb for image loading
- miniaudio for sound (though we also have openal-soft)
- openal-soft for audio (modified version that actually compiles)
- cpp-httplib for http stuff (downloading skins, etc)
- gzip-hpp for gzip compression

thats about it. if something breaks let me know i guess.
