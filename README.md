<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

<h1 align="center">
  <br>
  <a href="https://shadps4.net/"><img src="https://github.com/shadps4-emu/shadPS4/blob/main/.github/shadps4.png" width="220"></a>
  <br>
  <b>shadPS4</b>
  <br>
</h1>

<h1 align="center">
 <a href="https://discord.gg/bFJxfftGW6">
        <img src="https://img.shields.io/discord/1080089157554155590?color=5865F2&label=shadPS4 Discord&logo=Discord&logoColor=white" width="240">
 <a href="https://github.com/shadps4-emu/shadPS4/releases/latest">
        <img src="https://img.shields.io/github/downloads/shadps4-emu/shadPS4/total.svg" width="140">
 <a href="https://shadps4.net/">
        <img src="https://img.shields.io/badge/shadPS4-website-8A2BE2" width="150">
 <a href="https://x.com/shadps4">
        <img src="https://img.shields.io/badge/-Join%20us-black?logo=X&logoColor=white" width="100">
 <a href="https://github.com/shadps4-emu/shadPS4/stargazers">
        <img src="https://img.shields.io/github/stars/shadps4-emu/shadPS4" width="120">
</h1>

<p align="center">
  <a href="https://shadps4.net/">
  <img src="https://github.com/shadps4-emu/shadPS4/blob/main/documents/Screenshots/1.png" width="400">
  <img src="https://github.com/shadps4-emu/shadPS4/blob/main/documents/Screenshots/2.png" width="400">
  <img src="https://github.com/shadps4-emu/shadPS4/blob/main/documents/Screenshots/3.png" width="400">
  <img src="https://github.com/shadps4-emu/shadPS4/blob/main/documents/Screenshots/4.png" width="400">
</p>

# General information

**shadPS4** is an early **PlayStation 4** emulator for **Windows**, **Linux** and **macOS** written in C++.

If you encounter problems or have doubts, do not hesitate to look at the [**Quickstart**](https://github.com/shadps4-emu/shadPS4/blob/main/documents/Quickstart/Quickstart.md).

To verify that a game works, you can look at [**shadPS4 Game Compatibility**](https://github.com/shadps4-emu/shadps4-game-compatibility).

To discuss shadPS4 development, suggest ideas or to ask for help, join our [**Discord server**](https://discord.gg/bFJxfftGW6).

To get the latest news, go to our [**X (Twitter)**](https://x.com/shadps4) or our [**website**](https://shadps4.net/).

For those who'd like to donate to the project, we now have a [**Kofi page**](https://ko-fi.com/shadps4)!

# Status

> [!IMPORTANT]
> shadPS4 is early in development, don't expect a flawless experience.

Currently, the emulator successfully runs small games like [**Sonic Mania**](https://www.youtube.com/watch?v=AAHoNzhHyCU), [**Undertale**](https://youtu.be/5zIvdy65Ro4) and it can even run [**Bloodborne**](https://www.youtube.com/watch?v=wC6s0avpQRE).

# Why

This project began as a fun project. Given our limited free time, it may take some time before shadPS4 can run more complex games, but we're committed to making small, regular updates.

# Building

## Windows

Check the build instructions for [**Windows**](https://github.com/shadps4-emu/shadPS4/blob/main/documents/building-windows.md).

## Linux

Check the build instructions for [**Linux**](https://github.com/shadps4-emu/shadPS4/blob/main/documents/building-linux.md).

## macOS

Check the build instructions for [**macOS**](https://github.com/shadps4-emu/shadPS4/blob/main/documents/building-macos.md).

> [!IMPORTANT]
> macOS users need at least macOS 15 on Apple Silicon-based Mac devices and at least macOS 14 on Intel-based Mac devices.

# Debugging and reporting issues

For more information on how to test, debug and report issues with the emulator or games, read the [**Debugging documentation**](https://github.com/shadps4-emu/shadPS4/blob/main/documents/Debugging/Debugging.md).

# Keyboard mapping

> [!NOTE]
> Xbox and DualShock controllers work out of the box.

| Controller button | Keyboard equivelant |
|-------------|-------------|
LEFT AXIS UP | W |
LEFT AXIS DOWN | S |
LEFT AXIS LEFT | A |
LEFT AXIS RIGHT | D |
RIGHT AXIS UP | I |
RIGHT AXIS DOWN | K |
RIGHT AXIS LEFT | J |
RIGHT AXIS RIGHT | L |
TRIANGLE | Numpad 8 |
CIRCLE | Numpad 6 |
CROSS | Numpad 2 |
SQUARE | Numpad 4 |
PAD UP | UP |
PAD DOWN | DOWN |
PAD LEFT | LEFT |
PAD RIGHT | RIGHT |
OPTIONS | RETURN |
BACK BUTTON / TOUCH PAD | SPACE |
L1 | Q |
R1 | U |
L2 | E |
R2 | O |
L3 | X |
R3 | M |

# Main team

- [**georgemoralis**](https://github.com/georgemoralis)
- [**raphaelthegreat**](https://github.com/raphaelthegreat)
- [**psucien**](https://github.com/psucien)
- [**skmp**](https://github.com/skmp)
- [**wheremyfoodat**](https://github.com/wheremyfoodat)
- [**raziel1000**](https://github.com/raziel1000)

Logo is done by [**Xphalnos**](https://github.com/Xphalnos)

# Contributing

If you want to contribute, please look the [**CONTRIBUTING.md**](https://github.com/shadps4-emu/shadPS4/blob/main/CONTRIBUTING.md) file.

Open a PR and we'll check it :)

# Contributors

<a href="https://github.com/shadps4-emu/shadPS4/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=shadps4-emu/shadPS4&max=15">
</a>


# Special Thanks

A few noteworthy teams/projects who've helped us along the way are:

- [**Panda3DS**](https://github.com/wheremyfoodat/Panda3DS): A multiplatform 3DS emulator from our co-author wheremyfoodat. They have been incredibly helpful in understanding and solving problems that came up from natively executing the x64 code of PS4 binaries

- [**fpPS4**](https://github.com/red-prig/fpPS4): The fpPS4 team has assisted massively with understanding some of the more complex parts of the PS4 operating system and libraries, by helping with reverse engineering work and research.

- **yuzu**: Our shader compiler has been designed with yuzu's Hades compiler as a blueprint. This allowed us to focus on the challenges of emulating a modern AMD GPU while having a high-quality optimizing shader compiler implementation as a base.

- [**hydra**](https://github.com/hydra-emu/hydra): A multisystem, multiplatform emulator (chip-8, GB, NES, N64) from Paris.

# License

- [**GPL-2.0 license**](https://github.com/shadps4-emu/shadPS4/blob/main/LICENSE)
