<!--
SPDX-FileCopyrightText: 2025 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

# Build shadPS4 for Windows

This tutorial reads as if you have none of the prerequisites already installed. If you do, just ignore the steps regarding installation.
> [!WARNING]
> If you are trying to compile older builds for testing, do not provide the `--depth 1` flag in `git clone`.
> This flag omits the commit history from your clone, saving storage space while preventing you from testing older commits.

Note: **ARM64 is not supported!** As of writing, it will not build nor run. The instructions with respect to ARM64 are for developers only.

## Option 1: Visual Studio 2022

### (Prerequisite) Download the Community edition from [**Visual Studio 2022**](https://visualstudio.microsoft.com/vs/)

Once you are within the installer:

1. Select `Desktop development with C++`
2. Go to "Individual Components" tab then search and select both `C++ Clang Compiler for Windows` and `MSBuild support for LLVM`
3. Continue the installation

### (Prerequisite) Download [**Git for Windows**](https://git-scm.com/download/win)

Go through the Git for Windows installation as normal

### Cloning the source code

1. Open Git for Windows, navigate to a place where you want to store the shadPS4 source code folder
2. Clone the repository by running  
    `git clone --depth 1 --recursive https://github.com/shadps4-emu/shadPS4`

### Compiling with Visual Studio GUI

1. Open up Visual Studio, select `Open a local folder` and select the folder with the shadPS4 source code. The folder should contain `CMakeLists.txt`
2. Change Clang x64 Debug to Clang x64 Release if you want a regular, non-debug build.
3. Change the project to build to shadps4.exe
4. Build -> Build All

Your shadps4.exe will be in `C:\path\to\source\Build\x64-Clang-Release\`

## Option 2: MSYS2/MinGW

> [!IMPORTANT]
> Building with MSYS2 is broken as of right now, the only way to build on Windows is to use [Option 1: Visual Studio 2022](https://github.com/shadps4-emu/shadPS4/blob/main/documents/building-windows.md#option-1-visual-studio-2022).

### (Prerequisite) Download [**MSYS2**](https://www.msys2.org/)

Go through the MSYS2 installation as normal

If you are building to distribute, please omit `-DCMAKE_CXX_FLAGS="-O2 -march=native"` within the build configuration step.

Normal x86-based computers, follow:

1. Open "MSYS2 MINGW64" from your new applications
2. Run `pacman -Syu`, let it complete;
3. Run `pacman -S --needed git mingw-w64-x86_64-binutils mingw-w64-x86_64-clang mingw-w64-x86_64-cmake mingw-w64-x86_64-rapidjson mingw-w64-x86_64-ninja mingw-w64-x86_64-ffmpeg`
4. Run `git clone --depth 1 --recursive https://github.com/shadps4-emu/shadPS4`
5. Run `cd shadPS4`
6. Run `cmake -S . -B build -DCMAKE_C_COMPILER="clang.exe" -DCMAKE_CXX_COMPILER="clang++.exe" -DCMAKE_CXX_FLAGS="-O2 -march=native"`
7. Run `cmake --build build`
8. To run the finished product, run `./build/shadPS4.exe`

ARM64-based computers, follow:

1. Open "MSYS2 CLANGARM64" from your new applications
2. Run `pacman -Syu`, let it complete;
3. Run `pacman -S --needed git mingw-w64-clang-aarch64-binutils mingw-w64-clang-aarch64-clang mingw-w64-clang-aarch64-rapidjson mingw-w64-clang-aarch64-cmake mingw-w64-clang-aarch64-ninja mingw-w64-clang-aarch64-ffmpeg`
4. Run `git clone --depth 1 --recursive https://github.com/shadps4-emu/shadPS4`
5. Run `cd shadPS4`
6. Run `cmake -S . -B build -DCMAKE_C_COMPILER="clang.exe" -DCMAKE_CXX_COMPILER="clang++.exe" -DCMAKE_CXX_FLAGS="-O2 -march=native"`
7. Run `cmake --build build`
8. To run the finished product, run `./build/shadPS4.exe`

## Note on MSYS2 builds

These builds may not be easily copyable to people who do not also have a MSYS2 installation.
If you want to distribute these builds, you need to copy over the correct DLLs into a distribution folder.
In order to run them, you must be within the MSYS2 shell environment.
