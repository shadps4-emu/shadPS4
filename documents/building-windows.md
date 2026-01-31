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

## Option 2: VSCode with Visual Studio Build Tools

If your default IDE is VSCode, we have a fully functional example for that as well.

### Requirements

* [**Git for Windows**](https://git-scm.com/download/win)
* [**LLVM 19.1.1**](https://github.com/llvm/llvm-project/releases/download/llvmorg-19.1.1/LLVM-19.1.1-win64.exe)
* [**CMake 4.2.3 or newer**](https://github.com/Kitware/CMake/releases/download/v4.2.3/cmake-4.2.3-windows-x86_64.msi)
* [**Ninja 1.13.2 or newer**](https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-win.zip)

**The main reason we use clang19 is because that version is used in CI for formatting.**

### Installs

1. Go through the Git for Windows installation as normal
2. Download and Run LLVM Installer and `Add LLVM to the system PATH for all users`
3. Download and Run CMake Installer and `Add CMake to the system PATH for all users`
4. Download Ninja and extract it to `C:\ninja` and add it to the system PATH for all users    
    * You can do this by going to `Search with Start Menu -> Environment Variables -> System Variables -> Path -> Edit -> New -> C:\ninja`

### Validate the installs

```bash
git --version
# git version 2.49.0.windows.1

cmake --version
# cmake version 4.2.3

ninja --version
# 1.13.2

clang --version
# clang version 19.1.1
```

### Install Visual Studio Build Tools

1. Download [Visual Studio Build Tools](https://aka.ms/vs/17/release/vs_BuildTools.exe)
2. Select `MSVC - Windows SDK` and install (you don't need to install an IDE)

* Or you can install via `.vsconfig` file:

```
{
  "version": "1.0",
  "components": [
    "Microsoft.VisualStudio.Component.Roslyn.Compiler",
    "Microsoft.Component.MSBuild",
    "Microsoft.VisualStudio.Component.CoreBuildTools",
    "Microsoft.VisualStudio.Workload.MSBuildTools",
    "Microsoft.VisualStudio.Component.Windows10SDK",
    "Microsoft.VisualStudio.Component.VC.CoreBuildTools",
    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
    "Microsoft.VisualStudio.Component.VC.Redist.14.Latest",
    "Microsoft.VisualStudio.Component.Windows11SDK.26100",
    "Microsoft.VisualStudio.Component.TestTools.BuildTools",
    "Microsoft.VisualStudio.Component.VC.ASAN",
    "Microsoft.VisualStudio.Component.TextTemplating",
    "Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Core",
    "Microsoft.VisualStudio.Workload.VCTools"
  ],
  "extensions": []
}

Save the file as `.vsconfig` and run the following command:

%userprofile%\Downloads\vs_BuildTools.exe --passive --config ".vsconfig"

Be carefull path to vs_BuildTools.exe and .vsconfig file.
```

__This will install the necessary components to build shadPS4.__

### Project structure

```
shadps4/
  ├── shared (shadps4 main files)
  └── shadps4.code-workspace
```

### Content of `shadps4.code-workspace`

```json
{
  "folders": [
    {
      "path": "shared"
    }
  ],
  "settings": {
    "cmake.generator": "Ninja",
    
	"cmake.configureEnvironment": {
      "CMAKE_CXX_STANDARD": "23",
      "CMAKE_CXX_STANDARD_REQUIRED": "ON",
      "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
    },

    "cmake.configureOnOpen": false,

    "C_Cpp.intelliSenseEngine": "Disabled",

    "clangd.arguments": [
      "--background-index",
      "--clang-tidy",
      "--completion-style=detailed",
      "--header-insertion=never",
      "--compile-commands-dir=Build/x64-Clang-Release"
    ],

    "editor.formatOnSave": true,
    "clang-format.executable": "clang-format"
  },

  "extensions": {
    "recommendations": [
      "llvm-vs-code-extensions.vscode-clangd",
      "ms-vscode.cmake-tools",
      "xaver.clang-format"
    ]
  }
}
```

### Cloning the source code

1. Open your terminal and where to shadPS4 folder: `cd shadps4\shared`
3. Clone the repository by running  
    `git clone --depth 1 --recursive https://github.com/shadps4-emu/shadPS4 .`

_or fork link_

* If you have already cloned repo:
```bash
git submodule update --init --recursive
```

### Requirements VSCode extensions
1. CMake Tools
2. Clangd
3. Clang-Format

_These plugins are suggested in the workspace file above and are already configured._

![CMake Tools](https://raw.githubusercontent.com/shadps4-emu/shadPS4/refs/heads/main/documents/Screenshots/windows/vscode-ext-1.png)

![Clangd](https://raw.githubusercontent.com/shadps4-emu/shadPS4/refs/heads/main/documents/Screenshots/windows/vscode-ext-2.png)

![Clang Format](https://raw.githubusercontent.com/shadps4-emu/shadPS4/refs/heads/main/documents/Screenshots/windows/vscode-ext-3.png)

### Building
1. Open VS Code, `File > Open workspace from file > shadps4.code-workspace`
2. Go to the CMake Tools extension on left side bar
3. Change Clang x64 Debug to Clang x64 Release if you want a regular, non-debug build.
4. Click build.

Your shadps4.exe will be in `shadps4\shared\Build\x64-Clang-Release\`

## Option 3: MSYS2/MinGW

> [!IMPORTANT]
> Building with MSYS2 is broken as of right now, the only way to build on Windows is to use [Option 1: Visual Studio 2022](https://github.com/shadps4-emu/shadPS4/blob/main/documents/building-windows.md#option-1-visual-studio-2022) or [Option 2: VSCode with Visual Studio Build Tools](#option-2-vscode-with-visual-studio-build-tools).

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
