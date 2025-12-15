# OpenFunscripter
Can be used to create `.funscript` files. (NSFW)  
The project is based on OpenGL, SDL2, ImGui, libmpv, & all these other great [libraries](https://github.com/OpenFunscripter/OpenFunscripter/tree/master/lib).

![OpenFunscripter Screenshot](https://github.com/OpenFunscripter/OpenFunscripter/blob/1b4f096be8c2f6c75ceed7787a300a86a13fb167/OpenFunscripter.jpg)

Updates and New Features

New configuration file: The joystick_config.txt file has been added, containing the custom button mapping for a DirectInput controller, which is emulated as XInput within the application.

Controller module modification: The controller input module has been modified to support DirectInput controllers and map their buttons to a format compatible with XInput. This allows DirectInput controllers to be recognized and used as XInput controllers within the application.

Full-screen video mode: Added a windowed full-screen option for video playback.

The joystick_config.txt file must be placed in the same directory as the executable, as it is used to load the button mapping configuration when the application is run.

### How to build ( for people who want to contribute or fork )
1. Clone the repository
2. `cd "OpenFunscripter"`
3. `git submodule update --init`
4. Run CMake and compile

Known linux dependencies to just compile are `build-essential libmpv-dev libglvnd-dev`.  

### Windows libmpv binaries used
Currently using: [mpv-dev-x86_64-v3-20220925-git-56e24d5.7z (it's part of the repository)](https://sourceforge.net/projects/mpv-player-windows/files/libmpv/)

### Platforms
I'm providing windows binaries and a linux AppImage.
In theory OSX should work as well but I lack the hardware to set any of that up.
