<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

## Build shadPS4 for Linux

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
make -j$(nproc)
```
