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
- [**Configure the emulator**](#configure-the-emulator)

## Minimum PC requirements

### CPU

- A processor with at least 4 cores and 6 threads
- Above 2.5 GHz frequency
- A CPU supporting the x86-64-v3 baseline.
  - **Intel**: Haswell generation or newer
  - **AMD**: Excavator generation or newer
  - **Apple**: Rosetta 2 on macOS 15.4 or newer

### GPU

- A graphics card with at least 1GB of VRAM
- Up-to-date graphics drivers
- Vulkan 1.3 with the `VK_KHR_swapchain` and `VK_KHR_push_descriptor` extensions

### RAM

- 8GB of RAM or more

### OS

- Windows 10 or Ubuntu 22.04

## How to run the latest Work-in-Progress builds of shadPS4

1. Go to <https://github.com/shadps4-emu/shadPS4/releases> In the release identified as 'pre-release' click on the down arrow(Assets), select your operating system of choice (the "**qt**" versions have a user interface, which is probably the one you want. The others are SDL versions, which can only be run via command line).
![image](https://github.com/user-attachments/assets/af520c77-797c-41a0-8f67-d87f5de3e3df)

2. Once downloaded, extract to its own folder, and run shadPS4's executable from the extracted folder.

3. Upon first launch, shadPS4 will prompt you to select a folder to store your installed games in. Select "Browse" and then select a folder that contains your dumped games.

## Configure the emulator

To configure the emulator, you can go through the interface and go to "settings".

You can also configure the emulator by editing the `config.toml` file located in the `user` folder created after the application is started (Mostly useful if you are using the SDL version).
Some settings may be related to more technical development and debugging.\
For more information on this, see [**Debugging**](https://github.com/shadps4-emu/shadPS4/blob/main/documents/Debugging/Debugging.md#configuration).
