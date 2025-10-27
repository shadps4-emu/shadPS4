<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

## Build shadPS4 for macOS

### Install the necessary tools to build shadPS4:

First, make sure you have **Xcode 16.0 or newer** installed.

For installing other tools and library dependencies we will be using [Homebrew](https://brew.sh/).

On an ARM system, we will need the native ARM Homebrew to install tools and x86_64 Homebrew to install libraries.

First, install native Homebrew and tools:
```
# Installs native Homebrew to /opt/homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
# Adds Homebrew to your path
echo 'eval $(/opt/homebrew/bin/brew shellenv)' >> ~/.zprofile
eval $(/opt/homebrew/bin/brew shellenv)
# Installs tools.
brew install clang-format cmake
```

### Cloning and compiling:

Clone the repository recursively:
```
git clone --recursive https://github.com/shadps4-emu/shadPS4.git
cd shadPS4
```

Generate the build directory in the shadPS4 directory:
```
cmake -S . -B build/ -DCMAKE_OSX_ARCHITECTURES=x86_64
```

Enter the directory:
```
cd build/
```

Use make to build the project:
```
cmake --build . --parallel$(sysctl -n hw.ncpu)
```

Now run the emulator:

```
./shadps4 /"PATH"/"TO"/"GAME"/"FOLDER"/eboot.bin
```
