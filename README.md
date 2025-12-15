# OpenFunscripter
Can be used to create `.funscript` files. (NSFW)  
The project is based on OpenGL, SDL2, ImGui, libmpv, & all these other great [libraries](https://github.com/OpenFunscripter/OpenFunscripter/tree/master/lib).

![OpenFunscripter Screenshot](https://github.com/OpenFunscripter/OpenFunscripter/blob/1b4f096be8c2f6c75ceed7787a300a86a13fb167/OpenFunscripter.jpg)

Updates and New Features (This Fork)

DirectInput controller support:
Old DirectInput controllers are now recognized as XInput devices.

Windowed full-screen video mode: Added the ability to enter full-screen mode by double-clicking the video.

Joystick configuration file:
joystick_config.txt stores custom button mappings and must be in the same directory as the executable.

Default joystick axis centering:
The option to center the joystick axis is now unchecked by default.


### How to build ( for people who want to contribute or fork )
1. Clone the repository
2. `cd "OpenFunscripter"`
3. `git submodule update --init`
4. Run CMake and compile

Known linux dependencies to just compile are `build-essential libmpv-dev libglvnd-dev`.  

Windows libmpv Binaries (Important)

⚠️ Important: This has been tested with a pre-v3 version of libmpv using mpv-2.dll.  
v3 versions of libmpv **may not be compatible**.  
The library libmpv.dll is **not compatible** and will prevent the program from starting.  
If the program does not open and shows no error, it is likely due to using an incompatible version of libmpv.

Confirmed working version for this fork:

mpv-dev-x86_64-20221211-git-cf349d6.7z


https://sourceforge.net/projects/mpv-player-windows/files/libmpv/

### Platforms

Windows. Tested.
Linux or macOS. Not tested 