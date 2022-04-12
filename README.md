# WinQuake upgrade project

This project is a test to see what it would take to upgrade WinQuake to use modern game development methods.
It is not intended to be used for actual game development purposes.
If you are looking to make a Quake engine fork i would recommend checking out the Quake engine source ports that exist today.

# High-level overview

## This project uses CMake as a (meta-)build system.

CMake does a lot of work to set up project files for different build systems as well as different versions of them.
Over the course of development Visual Studios 2017, 2019 and 2022 as well as makefiles using G++ 9, 10 and 11 have been used without requiring any project configuration changes or upgrades.
CMake allows the debugging settings on Windows to be configured to target the game installation, which persists even after completely deleting the build directory and regenerating all files.

## All code has been converted to compile as C++.

Doing so allows the use of C++ standard library APIs such as `std::thread`, `std::chrono` and `std::filesystem`.
It also enforces stricter rules like no implicit conversions from `void*` and requiring functions to be forward declared.
Functions cannot be called with an arbitrary number of parameters unless they are using variadic argument syntax.
The use of default values for function parameters allows the removal of some global variables used to implement such behavior.

The codebase now uses C++17.

## The OpenGL renderer has been updated to work on modern systems.

As-is the game will crash on launch due to trying to print the value of `glGetString(GL_EXTENSIONS)` which now exceeds the console buffer.
Quake uses the older insecure string APIs from the C runtime library which do not check if an operation would write past the end of a buffer.

The game does not render correctly in OpenGL mode due to trying to use an old color table extension to reduce GPU memory usage.
Quake uses a shared palette that all textures reference. This extension allows images to reference the palette instead of containing the RGB values directly.
The extension seems to malfunction on newer systems causing textures to render as pure white (typically meaning texture data was not uploaded correctly).
Disabling this feature allows textures to render correctly.

## The Software renderer works without further changes.

The Software renderer's assembly code has been removed since its presence complicates some project configuration settings and requires changes to the C++ code to be mirrored to assembly code.
With modern CPUs being able to vectorize instructions, as well as compilers that can optimize code much more effectively this assembly code is no longer as efficient as it once was compared to the C++ version.

## The platform-specific window and OpenGL context management, keyboard, mouse and joystick/game controller code has been replaced with SDL2.

This simplifies most of the logic involved since SDL2 provides abstractions that hide most of the work that would otherwise need to be handled by programs.
Some system requirements have changed as a result of this work. You will need a display that supports 24 or more bits per pixel.

Linux now uses the same code as Windows for all of these systems.

## Software mode now renders to an OpenGL texture which is rendered as a fullscreen image.

The loading disc image is rendered on top of that when it is active.
This allows Software mode to work without depending on the MegaGraph Graphics Library, which is a precompiled static library exclusive to Windows.
This ensures all code is compiled with the same compiler and allows Software mode to work on Linux.

## The audio playback code has been updated to fall back to the fake direct memory access implementation if no audio system was able to initialize.

This is needed on Linux because its audio code relies on the obsolete `/dev/dsp` API.

## The engine has been updated to work better when run as a 64 bit program.

It is not possible to play games as-is because the precompiled QuakeC code uses 32 bit structure layouts and offsets, but it is possible to launch the game and interact with the main menu.

## More work to be done.

# Original readme.txt contents

This is the complete source code for winquake, glquake, quakeworld, and 
glquakeworld.

The projects have been tested with visual C++ 6.0, but masm is also required 
to build the assembly language files.  It is possible to change a #define and 
build with only C code, but the software rendering versions lose almost half 
its speed.  The OpenGL versions will not be effected very much.  The 
gas2masm tool was created to allow us to use the same source for the dos, 
linux, and windows versions, but I don't really recommend anyone mess 
with the asm code.

The original dos version of Quake should also be buildable from these 
sources, but we didn't bother trying.

The code is all licensed under the terms of the GPL (gnu public license).  
You should read the entire license, but the gist of it is that you can do 
anything you want with the code, including sell your new version.  The catch 
is that if you distribute new binary versions, you are required to make the 
entire source code available for free to everyone.

Our previous code releases have been under licenses that preclude 
commercial exploitation, but have no clause forcing sharing of source code.  
There have been some unfortunate losses to the community as a result of 
mod teams keeping their sources closed (and sometimes losing them).  If 
you are going to publicly release modified versions of this code, you must 
also make source code available.  I would encourage teams to even go a step 
farther and investigate using public CVS servers for development where 
possible.

The primary intent of this release is for entertainment and educational 
purposes, but the GPL does allow commercial exploitation if you obey the 
full license.  If you want to do something commercial and you just can't bear 
to have your source changes released, we could still negotiate a separate 
license agreement (for $$$), but I would encourage you to just live with the 
GPL.

All of the Quake data files remain copyrighted and licensed under the 
original terms, so you cannot redistribute data from the original game, but if 
you do a true total conversion, you can create a standalone game based on 
this code.

I will see about having the license changed on the shareware episode of 
quake to allow it to be duplicated more freely (for linux distributions, for 
example), but I can't give a timeframe for it.  You can still download one of 
the original quake demos and use that data with the code, but there are 
restrictions on the redistribution of the demo data.

If you never actually bought a complete version of Quake, you might want 
to rummage around in a local software bargain bin for one of the originals, 
or perhaps find a copy of the "Quake: the offering" boxed set with both 
mission packs.

Thanks to Dave "Zoid" Kirsh and Robert Duffy for doing the grunt work of 
building this release.

John Carmack
Id Software


