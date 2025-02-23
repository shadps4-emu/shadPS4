<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

## Build shadPS4 for Linux

First and foremost, Clang 18 is the **recommended compiler** as it is used for official builds and CI. If you build with GCC, you might encounter issues — please report any you find. Additionally, if you choose to use GCC, please build shadPS4 with Clang at least once before creating an `[APP BUG]` issue or submitting a pull request.

## Preparatory steps

### Installing dependencies

#### Debian & Ubuntu

```bash
sudo apt install build-essential clang git cmake libasound2-dev \
    libpulse-dev libopenal-dev libssl-dev zlib1g-dev libedit-dev \
    libudev-dev libevdev-dev libsdl2-dev libjack-dev libsndio-dev \
    qt6-base-dev qt6-tools-dev qt6-multimedia-dev libvulkan-dev \
    vulkan-validationlayers libpng-dev
```

#### Fedora

```bash
sudo dnf install clang git cmake libatomic alsa-lib-devel \
    pipewire-jack-audio-connection-kit-devel openal-devel \
    openssl-devel libevdev-devel libudev-devel libXext-devel \
    qt6-qtbase-devel qt6-qtbase-private-devel \
    qt6-qtmultimedia-devel qt6-qtsvg-devel qt6-qttools-devel \
    vulkan-devel vulkan-validation-layers libpng-devel
```

#### Arch Linux

```bash
sudo pacman -S base-devel clang git cmake sndio jack2 openal \
    qt6-base qt6-declarative qt6-multimedia qt6-tools sdl2 \
    vulkan-validation-layers libpng
```

**Note**: The `shadps4-git` AUR package is not maintained by any of the developers, and it uses the default compiler, which is often set to GCC. Use at your own discretion.

#### OpenSUSE

```bash
sudo zypper install clang git cmake libasound2 libpulse-devel \
    libsndio7 libjack-devel openal-soft-devel libopenssl-devel \
    zlib-devel libedit-devel systemd-devel libevdev-devel \
    qt6-base-devel qt6-multimedia-devel qt6-svg-devel \
    qt6-linguist-devel qt6-gui-private-devel vulkan-devel \
    vulkan-validationlayers libpng-devel
```

#### NixOS

```bash
nix-shell shell.nix
```

#### Other Linux distributions

You can try one of two methods:

- Search the packages by name and install them with your package manager, or
- Install [distrobox](https://distrobox.it/), create a container using any of the distributions cited above as a base, for Arch Linux you'd do:

```bash
distrobox create --name archlinux --init --image archlinux:latest
```

and install the dependencies on that container as cited above.
This option is **highly recommended** for distributions with immutable/atomic filesystems (example: Fedora Kinoite, SteamOS).

### Cloning

```bash
git clone --recursive https://github.com/shadps4-emu/shadPS4.git
cd shadPS4
```

## Building

There are 3 options you can choose from. Option 1 is **highly recommended**.

#### Option 1: Terminal-only

1. Generate the build directory in the shadPS4 directory.

```bash
cmake -S . -B build/ -DENABLE_QT_GUI=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

To disable the Qt GUI, remove the `-DENABLE_QT_GUI=ON` flag. To change the build type (for debugging), add `-DCMAKE_BUILD_TYPE=Debug`.

2. Use CMake to build the project:

```bash
cmake --build ./build --parallel$(nproc)
```

If your computer freezes during this step, this could be caused by excessive system resource usage. In that case, remove `--parallel$(nproc)`.

Now run the emulator. If Qt was enabled at configure time:

```bash
./build/shadps4
```

Otherwise, specify the path to your PKG's boot file:

```bash
./build/shadps4 /"PATH"/"TO"/"GAME"/"FOLDER"/eboot.bin
```

You can also specify the Game ID as an argument for which game to boot, as long as the folder containing the games is specified in config.toml (example: Bloodborne (US) is CUSA00900).
#### Option 2: Configuring with cmake-gui

`cmake-gui` should be installed by default alongside `cmake`, if not search for the package in your package manager and install it.

Open `cmake-gui` and specify the source code and build directories. If you cloned the source code to your Home directory, it would be `/home/user/shadPS4` and `/home/user/shadPS4/build`.

Click on Configure, select "Unix Makefiles", select "Specify native compilers", click Next and choose `clang` and `clang++` as the C and CXX compilers. Usually they are located in `/bin/clang` and `/bin/clang++`. Click on Finish and let it configure the project.

Now every option should be displayed in red. Change anything you want, then click on Generate to make the changes permanent, then open a terminal window and do step 2 of Option 1.

#### Option 3: Visual Studio Code

This option is pretty convoluted and should only be used if you have VSCode as your default IDE, or just prefer building and debugging projects through it. This also assumes that you're using an Arch Linux environment, as the naming for some options might differ from other distros.

[Download Visual Studio Code for your platform](https://code.visualstudio.com/download), or use [Code - OSS](https://github.com/microsoft/vscode) if you'd like. Code - OSS is available on most Linux distributions' package repositories (on Arch Linux it is simply named `code`).

Once set up, go to Extensions and install "CMake Tools":

![image](https://raw.githubusercontent.com/shadps4-emu/shadPS4/refs/heads/main/documents/Screenshots/Linux/3.png)

You can also install other CMake and Clang related extensions if you'd like, but this one is what enables you to configure and build CMake projects directly within VSCode.

Go to Settings, filter by `@ext:ms-vscode.cmake-tools configure` and disable this option:

![image](https://raw.githubusercontent.com/shadps4-emu/shadPS4/refs/heads/main/documents/Screenshots/Linux/1.png)

If you wish to build with the Qt GUI, add `-DENABLE_QT_GUI=ON` to the configure arguments:

![image](https://raw.githubusercontent.com/shadps4-emu/shadPS4/refs/heads/main/documents/Screenshots/Linux/2.png)

On the CMake tab, change the options as you wish, but make sure that it looks similar to or exactly like this:

![image](https://raw.githubusercontent.com/shadps4-emu/shadPS4/refs/heads/main/documents/Screenshots/Linux/4.png)

When hovering over Project Status > Configure, there should be an icon titled "Configure". Click on it and let it configure the project, then do the same for Project Status > Build.

If you want to debug it, change the build type under Project Status > Configure to Debug (it should be the default) and compile it, then click on the icon in Project Status > Debug. If you simply want to launch the shadPS4 executable from within VSCode, click on the icon in Project Status > Launch.

Don't forget to change the launch target for both options to the shadPS4 executable inside shadPS4/build:

![image](https://raw.githubusercontent.com/shadps4-emu/shadPS4/refs/heads/main/documents/Screenshots/Linux/5.png)
