<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

# shadPS4 Quickstart

## Summary

- [**PC Requirements**](#minimum-pc-requirements)
   - [**CPU**](#cpu)
   - [**GPU**](#gpu)
   - [**RAM**](#ram)
   - [**OS**](#os)
- [**Have the latest WIP version**](#how-to-run-the-latest-work-in-progress-builds-of-shadps4)
- [**Install PKG files (Games and Updates)**](#install-pkg-files)
- [**Configure the emulator**](#configure-the-emulator)

## Minimum PC requirements

### CPU

- A processor with at least 4 cores and 6 threads
- Above 2.5 GHz frequency
- A CPU supporting the following instruction sets: MMX, SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, F16C, CLMUL, AES, BMI1, MOVBE, XSAVE, ABM
  - **Intel**: Haswell generation or newer
  - **AMD**: Jaguar generation or newer
  - **Apple**: Rosetta 2 on macOS 15 or newer

### GPU

- A graphics card with at least 1GB of VRAM
- Keep your graphics drivers up to date
- Vulkan 1.3 support (required)

### RAM

- 8GB of RAM or more

### OS

- Windows 10 or Ubuntu 22.04

## How to run the latest Work-in-Progress builds of shadPS4

1. Go to <https://github.com/shadps4-emu/shadPS4/releases> In the release identified as 'pre-release' click on the down arrow(Assets), select your operating system of choice (the "**qt**" versions have a user interface, which is probably the one you want. The others are SDL versions, which can only be run via command line).
![image](https://github.com/user-attachments/assets/af520c77-797c-41a0-8f67-d87f5de3e3df)

2. Once downloaded, extract to its own folder, and run shadPS4's executable from the extracted folder.

3. Upon first launch, shadPS4 will prompt you to select a folder to store your installed games in. Select "Browse" and then select a folder that shadPS4 can use to install your PKG files to.

## Install PKG files

To install PKG files (game and updates), you will need the Qt application (with UI). You will have to go to "File" then to "Install Packages (PKG)", a window will open then you will have to select the files. You can install multiple PKG files at once. Once finished, the game should appear in the application.

<img src="https://github.com/shadps4-emu/shadPS4/blob/main/documents/Quickstart/2.png" width="800">

## Configure the emulator

To configure the emulator, you can go through the interface and go to "settings".

You can also configure the emulator by editing the `config.toml` file located in the `user` folder created after the application is started (Mostly useful if you are using the SDL version).
Some settings may be related to more technical development and debugging.\
For more information on this, see [**Debugging**](https://github.com/shadps4-emu/shadPS4/blob/main/documents/Debugging/Debugging.md#configuration).