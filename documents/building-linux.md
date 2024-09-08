<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

## Build shadPS4 for Linux

### Install the necessary tools to build shadPS4:

#### Debian & Ubuntu
```
sudo apt-get install build-essential libasound2-dev libpulse-dev libopenal-dev zlib1g-dev libedit-dev libvulkan-dev libudev-dev git libevdev-dev libsdl2-2.0 libsdl2-dev libjack-dev libsndio-dev qt6-base-dev qt6-tools-dev
```

#### Fedora
```
sudo dnf install alsa-lib-devel cmake libatomic libevdev-devel libudev-devel openal-devel qt6-qtbase-devel qt6-qtbase-private-devel vulkan-devel pipewire-jack-audio-connection-kit-devel qt6-qtmultimedia-devel qt6-qtsvg-devel
```

#### Arch Linux
```
sudo pacman -S openal cmake vulkan-validation-layers qt6-base qt6-declarative qt6-multimedia sdl2 sndio jack2 base-devel
```

#### OpenSUSE
```
sudo zypper install git cmake libasound2 libpulse-devel openal-soft-devel zlib-devel libedit-devel vulkan-devel libudev-devel libqt6-qtbase-devel libqt6-qtmultimedia-devel libqt6-qtsvg-devel libQt6Gui-private-headers-devel libevdev-devel libsndio7_1 libjack-devel
```
### Cloning and compiling:

Clone the repository recursively:
```
git clone --recursive https://github.com/shadps4-emu/shadPS4.git
cd shadPS4
```

Generate the build directory in the shadPS4 directory. To enable the QT GUI, pass the ```-DENABLE_QT_GUI=ON``` flag:
```
cmake -S . -B build/ -DENABLE_QT_GUI=ON
```

Enter the directory:
```
cd build/
```

Use make to build the project:
```
cmake --build . --parallel$(nproc)
```

Now run the emulator. If QT is enabled:
```
./shadps4
```
Otherwise, specify the path to your PKG's boot file:
```
./shadps4 /"PATH"/"TO"/"GAME"/"FOLDER"/eboot.bin
```
