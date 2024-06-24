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

## Have the latest WIP version

When you go to Github Release, you have the latest major versions (e.g. v0.0.3), but if you want to have the latest Work-In-Progress version, you can go to Actions on Github to download it (Please note a Github account is required to be able to download).

<img src="https://github.com/shadps4-emu/shadPS4/blob/main/documents/Quickstart/1.png" width="800"></a>

After downloading the version suitable for you (Windows or Linux), you must unzip the file and then you can run it. Please note, there are two versions for each platform, a Qt version with user interface and one without (SDL Builds).

## Install PKG files

To install PKG files (game and updates), you will need the Qt application (with UI). You will have to go to "File" then to "Install Packages (PKG)", a window will open then you will have to select the files. You can install multiple PKG files at once. Once finished, the game should appear in the application.

<img src="https://github.com/shadps4-emu/shadPS4/blob/main/documents/Quickstart/2.png" width="800"></a>

## Configure the emulator

You can configure the emulator in the "user" folder (created after the first start of the application) then in the "config.toml" file. Here you can find lots of parameters to set with True or False.
