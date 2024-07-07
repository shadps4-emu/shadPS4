<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

## Build shadPS4 for Linux

Clone the repository recursively:
```
git clone https://github.com/shadps4-emu/shadPS4.git
cd shadPS4/
git submodule update --init --recursive
```

Generate the build directory in the shadPS4 directory:
```
cmake -S . -B build/
```

Enter the directory:
```
cd build/
```

Use make to build the project:
```
cmake /"PATH"/"TO"/"DIRECTORY"/shadPS4/ && make -j$(nproc)
```

Now run the emulator:

```
./shadps4 /"PATH"/"TO"/"GAME"/"FOLDER"/eboot.bin
```
