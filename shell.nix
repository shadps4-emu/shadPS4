# SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

with import (fetchTarball "https://github.com/nixos/nixpkgs/archive/bd40c4ee221df5f3554d6f7b895a513be16f2e89.tar.gz") { };

clangStdenv.mkDerivation {
  name = "shadps4-build-env";

  nativeBuildInputs = [
    pkgs.pkgs.clangStdenv
    pkgs.cmake
    pkgs.pkg-config
    pkgs.git
  ];

  buildInputs = [
    pkgs.alsa-lib
    pkgs.libpulseaudio
    pkgs.pipewire
    pkgs.openal
    pkgs.openssl
    pkgs.zlib
    pkgs.libedit
    pkgs.udev
    pkgs.libevdev
    pkgs.SDL2
    pkgs.jack2
    pkgs.sndio
    pkgs.qt6.qtbase
    pkgs.qt6.qttools
    pkgs.qt6.qtmultimedia

    pkgs.vulkan-headers
    pkgs.vulkan-utility-libraries
    pkgs.vulkan-tools

    pkgs.ffmpeg
    pkgs.fmt
    pkgs.glslang
    pkgs.libxkbcommon
    pkgs.wayland
    pkgs.xorg.libxcb
    pkgs.xorg.xcbutil
    pkgs.xorg.xcbutilkeysyms
    pkgs.xorg.xcbutilwm
    pkgs.sdl3
    pkgs.stb
    pkgs.qt6.qtwayland
    pkgs.wayland-protocols
    pkgs.libpng
  ];

  shellHook = ''
    echo "Entering shadPS4 dev shell"
    export QT_QPA_PLATFORM="wayland"
    export QT_PLUGIN_PATH="${pkgs.qt6.qtwayland}/lib/qt-6/plugins:${pkgs.qt6.qtbase}/lib/qt-6/plugins"
    export QML2_IMPORT_PATH="${pkgs.qt6.qtbase}/lib/qt-6/qml"
    export CMAKE_PREFIX_PATH="${pkgs.vulkan-headers}:$CMAKE_PREFIX_PATH"
    export LC_ALL="C.UTF-8"
    export XAUTHORITY=${builtins.getEnv "XAUTHORITY"}
  '';
}
