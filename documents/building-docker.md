<!--
SPDX-FileCopyrightText: 2026 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

# Building shadPS4 with Docker and VSCode Support

This guide explains how to build **shadPS4** using Docker while keeping full compatibility with **VSCode** development.

---

## Prerequisites

Before starting, ensure you have:

- **Docker Engine** or **Docker Desktop** installed  
  [Installation Guide](https://docs.docker.com/engine/install/)

- **Git** installed on your system.

---

## Step 1: Prepare the Docker Environment

Inside the container (or on your host if mounting volumes):

1. Navigate to the repository folder containing the Docker Builder folder:

```bash
cd <path-to-repo>
```

2. Start the Docker container:

```bash
docker compose up -d
```

This will spin up a container with all the necessary build dependencies, including Clang, CMake, SDL2, Vulkan, and more.

## Step 2: Clone shadPS4 Source

```bash
mkdir emu
cd emu
git clone --recursive https://github.com/shadps4-emu/shadPS4.git .

or your fork link.
```

3. Initialize submodules:

```bash
git submodule update --init --recursive
```

## Step 3: Build with CMake

Generate the build directory and configure the project using Clang:

```bash
cmake -S . -B build/ -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

Then build the project:

```bash
cmake --build ./build --parallel $(nproc)
```

* Tip: To enable debug builds, add -DCMAKE_BUILD_TYPE=Debug to the CMake command.

---

After a successful build, the executable is located at:

```bash
./build/shadps4
```

## Step 4: VSCode Integration

1. Open the repository in VSCode.
2. The CMake Tools extension should automatically detect the build directory inside the container or on your host.
3. You can configure build options, build, and debug directly from the VSCode interface without extra manual setup.

# Notes

* The Docker environment contains all dependencies, so you don’t need to install anything manually.
* Using Clang inside Docker ensures consistent builds across Linux and macOS runners.
* GitHub Actions are recommended for cross-platform builds, including Windows .exe output, which is not trivial to produce locally without Visual Studio or clang-cl.