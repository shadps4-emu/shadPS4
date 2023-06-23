# shadPS4

An early PS4 emulator for Windows and Linux


[Check us on twitter](https://twitter.com/shadps4 "Check us on twitter")

# Status

Currently it can only load PS4 ELF files.

![](https://geps.dev/progress/60) Elf Loader

![](https://geps.dev/progress/20) Logging system

![](https://geps.dev/progress/5) Everything else

# Why?

The project started as a fun project. Due to short amount of free time probably it will take a while since it will be able to run something decent but I am trying to do regular small commits.

# Build

## Windows

The project is using cmake files. To build, just use Visual Studio 2022.

## Linux

Generate the build directory in the shadPS4 directory:
```
cmake -S . -B build/
```

Enter the directory:
```
cd build/
```

Use make to build the project:
```
make -j$(nproc)
```

|Platform|Build status|
|--------|------------|
|Windows build|[![Windows](https://github.com/georgemoralis/shadPS4/actions/workflows/windows.yml/badge.svg)](https://github.com/georgemoralis/shadPS4/actions/workflows/windows.yml)
|Linux build| TODO


To discuss this emulator please join our Discord server: [![Discord](https://img.shields.io/discord/1080089157554155590)](https://discord.gg/MyZRaBngxA)

# Who are you?

Old emulator fans and devs can recongnize me as "shadow". I was the founder and coder for a lot of emulation projects:
* PCSX
* PCSX2
* PCSP
* JPCSP
* arcadeflex
* rpcs3 contributor

# Contribution

I currently accept any kind of contribution, here is a list of some items that may be useful:

* PKG extractor (there was an initial work on this, just search project history commits).
* Initial GUI with imgui, SDL3 and Vulkan.
* Better logging system with spdlog.
* to be filled...

# Documentation

Wiki has some documentation for PS4 PKG format

[PKG PS4 File Format](https://github.com/georgemoralis/shadPS4/wiki/PKG-Information "PKG PS4 File Format")

