# shadPS4

An early PS4 emulator for Windows and Linux


[Check us on twitter](https://twitter.com/shadps4 "Check us on twitter")

# Status

Currently it can only load PS4 ELF files.

Progress is focus on videoout_basic.elf from sdk demos , currently it can load and run a few HLE calls. Others probably won't run since they might not be able to relocate all neccesary functions

![](https://geps.dev/progress/60) Elf Loader

![](https://geps.dev/progress/20) Logging system

![](https://geps.dev/progress/10) Everything else

# Why?

The project started as a fun project. Due to short amount of free time probably it will take a while since it will be able to run something decent but I am trying to do regular small commits.

# Build

## Windows

Check building instructions in [windows build](https://github.com/georgemoralis/shadPS4/edit/main/documents/building-windows.md)

## Linux

Check building instructions in [linux build](https://github.com/georgemoralis/shadPS4/blob/main/documents/linux_building.md)

## Build status

|Platform|Build status|
|--------|------------|
|Windows build|[![Windows](https://github.com/georgemoralis/shadPS4/actions/workflows/windows.yml/badge.svg)](https://github.com/georgemoralis/shadPS4/actions/workflows/windows.yml)
|Linux build|[![Linux](https://github.com/georgemoralis/shadPS4/actions/workflows/linux.yml/badge.svg)](https://github.com/georgemoralis/shadPS4/actions/workflows/linux.yml)


To discuss this emulator please join our Discord server: [![Discord](https://img.shields.io/discord/1080089157554155590)](https://discord.gg/MyZRaBngxA)

# Who are you?

Old emulator fans and devs can recognize me as "shadow". I was the founder and coder for a lot of emulation projects:
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

