# Retro League GX
Retro League GX is a demake of Rocket League for older game consoles.

## Windows Setup

1. Make sure to clone submodules when cloning the repo. This repo depends on the Octave game engine `git clone --recurse-submodules`  
2. Follow the Octave setup instructions to make sure you have all of the libraries and prerequisites installed (https://github.com/mholtkamp/octave)
3. Make sure you have compiled shaders for Octave by running Octave/Engine/Shaders/GLSL/compile.bat
4. Open the RetroLeague.sln in Visual Studio Community 2022
5. Set Rocket as the Startup Project
6. Set Rocket's Debug working directory to `$(SolutionDir)Octave`
7. Set Rocket's Command Arguments to `-project ../Rocket/Rocket.octp`
8. Launch Rocket in DebugEditor to open the editor
9. Package the project for Windows using `File->Package Project->Windows`. This is needed to generate engine asset files before running the game.
10. Launch Rocket in Debug to run the game
11. If you want to package the game for Windows with `File->Package Project` in the Editor, add devenv to the PATH. My devenv for VS 2022 community was located here: `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE`. The resulting "packaged" game will be in the Packaged folder in the reposity root.


## Linux Setup

1. Make sure to clone submodules when cloning the repo. This repo depends on the Octave game engine `git clone --recurse-submodules`  
2. Follow the Octave setup instructions to make sure you have all of the libraries and prerequisites installed (https://github.com/mholtkamp/octave)
3. Make sure you have compiled shaders for Octave by running Octave/Engine/Shaders/GLSL/compile.sh
4. Go into the Rocket directory and run `make -f Makefile_Linux_Editor` to compile the editor
5. Run `make -f Makefile_Linux_Game` to compile the game
6. Go into the Octave directory and run `../Rocket/Build/Linux/RocketEditor.out -project ../Rocket/Rocket.octp` to run the editor and automatically load the RetroLeague project (codename Rocket)
7. Package the project for Linux using `File->Package Project->Linux`. This is needed to generate engine asset files before running the game.
8. Run `../Rocket/Build/Linux/Rocket.out -project ../Rocket/Rocket.octp` to run the game
9. You can package the project in the Editor by selecting `File->Package Project->Linux` and this will create a "Packaged" in the root directory that is easier to distribute.

### Linux Setup (VsCode)
Alternatively to compiling and executing manually, you can instead open the root folder in Visual Studio Code and you should be able to run the `Rocket Editor` and `Rocket Game` tasks to compile and launch the game with the correct working directory and project commandline arg.
