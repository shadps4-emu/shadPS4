# SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

with import (fetchTarball "https://github.com/nixos/nixpkgs/archive/cfd19cdc54680956dc1816ac577abba6b58b901c.tar.gz") { };

pkgs.mkShell {
  name = "shadps4-build-env";

  nativeBuildInputs = with pkgs; [
    llvmPackages_18.clang
    cmake
    pkg-config
    git
    util-linux
  ];

  buildInputs = with pkgs; [
    alsa-lib
    libpulseaudio
    openal
    zlib
    libedit
    udev
    libevdev
    SDL2
    jack2
    sndio

    vulkan-headers
    vulkan-utility-libraries
    vulkan-tools

    ffmpeg
    fmt
    glslang
    libxkbcommon
    wayland
    xorg.libxcb
    xorg.xcbutil
    xorg.xcbutilkeysyms
    xorg.xcbutilwm
    sdl3
    stb
    wayland-protocols
    libpng
  ];

  shellHook = ''
    echo "Entering shadPS4 dev shell"
    export CMAKE_PREFIX_PATH="${pkgs.vulkan-headers}:$CMAKE_PREFIX_PATH"

    # OpenGL
    export LD_LIBRARY_PATH="${
      pkgs.lib.makeLibraryPath [
        pkgs.libglvnd
        pkgs.vulkan-tools
      ]
    }:$LD_LIBRARY_PATH"

    export LDFLAGS="-L${pkgs.llvmPackages_18.libcxx}/lib -lc++"
    export LC_ALL="C.UTF-8"
    export XAUTHORITY=${builtins.getEnv "XAUTHORITY"}
  '';
}
