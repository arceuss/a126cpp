# a126cpp

c++ port of minecraft alpha 1.2.6. its basically done except for some niche stuff that most people wont notice. performance is still kinda shit compared to the original java but ram usage is fine, only uses like 600mb max.

## building

you need cmake 3.15+ and a c++ compiler. on windows you can use visual studio or just gcc if you have it. all the dependencies are in the `external/` folder so you dont need to install anything else.

to build:
1. make a build directory: `mkdir build && cd build`
2. run cmake: `cmake ..` (or `cmake -G "Visual Studio 17 2022" ..` for visual studio)
3. build it: `cmake --build .` (or just open the .sln in visual studio)

the executable will be in `bin/Alpha126Cpp.exe` (or just `Alpha126Cpp` on linux).

## resources

you need to extract the resources from the original minecraft alpha 1.2.6 jar into a `resources` folder next to the executable. the cmake build should copy the resources folder automatically but if it doesnt, just make sure the `resources` folder is in the same directory as the exe.

## dependencies

everything is included as submodules or in the external folder:
- SDL2 for windowing and input
- zlib for compression
- stb for image loading
- miniaudio for sound (though we also have openal-soft)
- openal-soft for audio (modified version that actually compiles)
- cpp-httplib for http stuff (downloading skins, etc)
- gzip-hpp for gzip compression

## known issues

- framerate can drop in big city areas on servers, still working on that
- some lighting stuff might be slightly off in multiplayer but its close enough
- the code is pretty spaghetti but it works

thats about it. if something breaks let me know i guess.
