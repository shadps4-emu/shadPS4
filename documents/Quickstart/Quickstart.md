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

### GPU

- A graphics card with at least 1GB of VRAM
- Keep your graphics drivers up to date
- Vulkan 1.3 support (required)

### RAM

- 8GB of RAM or more

### OS

- Windows 10 or Ubuntu 22.04

## How to run the latest Work-in-Progress builds of ShadPS4

1. Go to <https://github.com/shadps4-emu/shadPS4/actions> and make sure you are logged into your GitHub account (important!)
2. On the left side of the page, select your operating system of choice (the "**qt**" versions have a user interface, which is probably the one you want. The others are SDL versions, which can only be run via command line). ![image](https://github.com/user-attachments/assets/43f01bbf-236c-4d6d-98ac-f5a5badd4ce8)

3. In the workflow list, select the latest entry with a green :white_check_mark: icon in front of it. (or the latest entry for whatever pull request you wish to test). ![image](https://github.com/user-attachments/assets/6365f407-867c-44ae-bf00-944f8d84a349)

4. On the bottom of this page, select the name of the file, and it should start downloading. (If there is no file here, double check that you are indeed logged into a GitHub account, and that there is a green :white_check_mark: icon. ![image](https://github.com/user-attachments/assets/97924500-3911-4f90-ab63-ffae7e52700b)

5. Once downloaded, extract to its own folder, and run ShadPS4's executable from the extracted folder.

6. Upon first launch, ShadPS4 will prompt you to select a folder to store your installed games in. Select "Browse" and then select a folder that ShadPS4 can use to install your PKG files to.

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