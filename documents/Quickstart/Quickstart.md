<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

# shadPS4 Quickstart

## Summary

- [PC Requirements](#pc-requirements)
   - [CPU](#cpu)
   - [GPU](#gpu)
   - [RAM](#ram)
   - [OS](#os)
- [Have the latest WIP version](#have-the-latest-wip-version)
- [Install PKG files (Games and Updates)](#install-pkg-files)
- [Configure the emulator](#configure-the-emulator)

## PC Requirements

### CPU

- A processor with at least 4 cores and 6 threads
- Above 2.5 GHz frequency
- required support AVX2 extension or Rosetta 2 on ARM

### GPU

- A graphics card with at least 1GB of VRAM
- Keep your graphics drivers up to date
- Vulkan 1.3 support (required)

### RAM

- 8GB of RAM or more

### OS

- Windows 10 or Ubuntu 22.04

## How to run the latest Work-in-Progress builds of ShadPS4

1. Go to <https://github.com/shadps4-emu/shadPS4/releases> In the release identified as 'pre-release' click on the down arrow(Assets), select your operating system of choice (the "**qt**" versions have a user interface, which is probably the one you want. The others are SDL versions, which can only be run via command line).
![image](https://github.com/user-attachments/assets/af520c77-797c-41a0-8f67-d87f5de3e3df)

2. Once downloaded, extract to its own folder, and run ShadPS4's executable from the extracted folder.

3. Upon first launch, ShadPS4 will prompt you to select a folder to store your installed games in. Select "Browse" and then select a folder that ShadPS4 can use to install your PKG files to.

## Install PKG files

To install PKG files (game and updates), you will need the Qt application (with UI). You will have to go to "File" then to "Install Packages (PKG)", a window will open then you will have to select the files. You can install multiple PKG files at once. Once finished, the game should appear in the application.

<img src="https://github.com/shadps4-emu/shadPS4/blob/main/documents/Quickstart/2.png" width="800"></a>

## Configure the emulator

You can configure the emulator by editing the `config.toml` file found in the `user` folder created after starting the application.\
Some settings may be related to more technical development and debugging. For more information on those, see [Debugging](https://github.com/shadps4-emu/shadPS4/blob/main/documents/Debugging/Debugging.md#configuration).

Here's a list of configuration entries that are worth changing:

- `[General]`

  - `Fullscreen`: Display the game in a full screen borderless window.
  
  - `logType`: Configures logging synchronization (`sync`/`async`)
    - It can be beneficial to set this to `sync` in order for the log to accurately maintain message order, at the cost of performance.
    - Use when sending logs to developers. See more about [reporting issues](https://github.com/shadps4-emu/shadPS4/blob/main/documents/Debugging/Debugging.md#reporting-and-communicating-about-issues).
  - `logFilter`: Sets the logging category for various logging classes.
    - Format: `<class>:<level> ...`, `<class.*>:<level> <*:level> ...`
    - Valid log levels: `Trace, Debug, Info, Warning, Error, Critical` - in this order, setting a level silences all levels preceding it and logs every level after it.
    - Examples:
      - If the log is being spammed with messages coming from Lib.Pad, you can use `Lib.Pad:Critical` to only log critical-level messages.
      - If you'd like to mute everything, but still want to receive messages from Vulkan rendering: `*:Error Render.Vulkan:Info`
    
- `[GPU]`
  - `screenWidth` and `screenHeight`: Configures the game window width and height.
