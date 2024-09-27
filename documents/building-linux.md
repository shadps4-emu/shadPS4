<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

## Build shadPS4 for Linux                                      

### Install the necessary tools to build shadPS4:

#### Debian & Ubuntu
```
sudo apt install build-essential clang git cmake libasound2-dev libpulse-dev libopenal-dev libssl-dev zlib1g-dev libedit-dev libudev-dev libevdev-dev libsdl2-dev libjack-dev libsndio-dev qt6-base-dev qt6-tools-dev qt6-multimedia-dev libvulkan-dev vulkan-validationlayers
```

#### Fedora
```
sudo dnf install clang cmake libatomic alsa-lib-devel pipewire-jack-audio-connection-kit-devel openal-devel openssl-devel libevdev-devel libudev-devel qt6-qtbase-devel qt6-qtbase-private-devel qt6-qtmultimedia-devel qt6-qtsvg-devel qt6-qttools-devel vulkan-devel vulkan-validation-layers
```

#### Arch Linux
```
sudo pacman -S base-devel clang git cmake sndio jack2 openal qt6-base qt6-declarative qt6-multimedia sdl2 vulkan-validation-layers
```

#### OpenSUSE
```
sudo zypper install clang git cmake libasound2 libpulse-devel libsndio7 libjack-devel openal-soft-devel libopenssl-devel zlib-devel libedit-devel systemd-devel libevdev-devel qt6-base-devel qt6-multimedia-devel qt6-svg-devel qt6-linguist-devel qt6-gui-private-devel vulkan-devel vulkan-validationlayers
```
### Cloning and compiling:

Clone the repository recursively:
```
git clone --recursive https://github.com/shadps4-emu/shadPS4.git
cd shadPS4
```

Generate the build directory in the shadPS4 directory. To disable the QT GUI, remove the ```-DENABLE_QT_GUI=ON``` flag:

**Note**: Clang is the compiler used for official builds and CI. If you build with GCC, you might encounter issues—please report any you find. If you choose to use GCC, we recommend building with Clang at least once before submitting a pull request.
```
cmake -S . -B build/ -DENABLE_QT_GUI=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
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
