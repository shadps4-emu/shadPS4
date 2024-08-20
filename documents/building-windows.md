<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

# Build shadPS4 for Windows

This tutorial reads as if you have none of the prerequisites already installed. If you do, just ignore the steps regarding installation.
If you are building to contribute to the project, please omit `--depth 1` from the git invokations.

Note: **ARM64 is not supported!** As of writing, it will not build nor run. The instructions with respect to ARM64 are for developers only.

## Option 1: Visual Studio 2022

### (Prerequisite) Download the Community edition from [**Visual Studio 2022**](https://visualstudio.microsoft.com/vs/)

Once you are within the installer:
1. Select `Desktop development with C++`
2. Go to "Individual Components" tab
3. Make sure `C++ Clang Compiler for Windows`, `MSBuild support for LLVM` and `C++ CMake Tools for Windows` are selected
4. Continue the installation

### (Prerequisite) Download [**Qt**](https://doc.qt.io/qt-6/get-and-install-qt.html)

Beware, this requires you to create a Qt account. If you do not want to do this, please follow the MSYS2/MinGW compilation method instead.

1. Select Qt for Visual Studio plugin
2. Select `msvc2019_64` option or similar. If you are on Windows on ARM / Qualcomm Snapdragon Elite X, select `msvc2019_arm64`

Go through the installation normally. If you do not know what components to select, just select the newest Qt version it gives you.
If you know what you are doing, you may unselect individual components that eat up too much disk space.

Once you are finished, you will have to configure Qt within Visual Studio:
1. Tools -> Options -> Qt -> Versions
2. Add a new Qt version and navigate it to the correct folder. Should look like so: `C:\Qt\6.7.1\msvc2019_64` 
3. Enable the default checkmark on the new version you just created. 

### (Prerequisite) Download [**Git for Windows**](https://git-scm.com/download/win)

Go through the Git for Windows installation as normal

### Compiling with Visual Studio GUI

1. Open Git for Windows, navigate to a place where you want to store the shadPS4 source code folder
2. Run `git clone --depth 1 --recursive https://github.com/shadps4-emu/shadPS4`
3. Open up Visual Studio, select `Open a local folder` and select the folder with the shadPS4 source code. The folder should contain `CMakeLists.txt`
4. Build -> Build All

## Option 2: MSYS2/MinGW

### (Prerequisite) Download [**MSYS2**](https://www.msys2.org/)

Go through the MSYS2 installation as normal

If you are building to distribute, please omit `-DCMAKE_CXX_FLAGS="-O2 -march=native"` within the build configuration step.

Normal x86-based computers, follow:
1. Open "MSYS2 MINGW64" from your new applications
2. Run `pacman -Syu`, let it complete;
3. Run `pacman -S --needed git mingw-w64-x86_64-binutils mingw-w64-x86_64-clang mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-qt6-base`
4. Run `git clone --depth 1 --recursive https://github.com/shadps4-emu/shadPS4`
5. Run `cd shadPS4`
6. Run `cmake -S . -B build -DCMAKE_CXX_COMPILER="clang++.exe" -DCMAKE_C_COMPILER="clang.exe" -DCMAKE_CXX_FLAGS="-O2 -march=native"`
7. Run `cmake --build build`
8. To run the finished product, run `./build/shadPS4.exe`

ARM64-based computers, follow:
1. Open "MSYS2 CLANGARM64" from your new applications
2. Run `pacman -Syu`, let it complete;
3. Run `pacman -S --needed git mingw-w64-clang-aarch64-binutils mingw-w64-clang-aarch64-clang mingw-w64-clang-aarch64-cmake mingw-w64-clang-aarch64-ninja mingw-w64-clang-aarch64-qt6-base`
4. Run `git clone --depth 1 --recursive https://github.com/shadps4-emu/shadPS4`
5. Run `cd shadPS4`
6. Run `cmake -S . -B build -DCMAKE_CXX_COMPILER="clang++.exe" -DCMAKE_C_COMPILER="clang.exe" -DCMAKE_CXX_FLAGS="-O2 -march=native"`
7. Run `cmake --build build`
8. To run the finished product, run `./build/shadPS4.exe`

## Note on MSYS2 builds

These builds may not be easily copyable to people who do not also have a MSYS2 installation.
If you want to distribute these builds, you need to copy over the correct DLLs into a distribution folder.
In order to run them, you must be within the MSYS2 shell environment.