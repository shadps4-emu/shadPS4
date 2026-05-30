## SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
## SPDX-License-Identifier: GPL-2.0-or-later

{ clangStdenv
, cmake
, ninja
, pkg-config
, magic-enum
, fmt
, eudev
, boost
, cli11
, openal
, nlohmann_json
, vulkan-loader
, vulkan-headers
, vulkan-memory-allocator
, toml11
, zlib
, zydis
, pugixml
, ffmpeg
, libpulseaudio
, pipewire
, wayland
, wayland-scanner
, libX11
, libxrandr
, libxext
, libxcursor
, libxi
, libxscrnsaver
, libxtst
, libxcb
, libdecor
, libxkbcommon
, libGL
, libuuid
, miniz
, libressl
, src
, system
, cmakeFlags
, dontStrip ? true
,
}:

clangStdenv.mkDerivation (finalAttrs: {
  inherit src system cmakeFlags dontStrip;

  pname = "shadps4";
  version = "0.15.1";

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
    magic-enum
    fmt
    eudev
  ];
  buildInputs = [
    boost
    cli11
    openal
    nlohmann_json
    vulkan-loader
    vulkan-headers
    vulkan-memory-allocator
    toml11
    zlib
    zydis
    pugixml
    ffmpeg
    libpulseaudio
    pipewire
    wayland
    wayland-scanner
    libX11
    libxrandr
    libxext
    libxcursor
    libxi
    libxscrnsaver
    libxtst
    libxcb
    libdecor
    libxkbcommon
    libGL
    libuuid
    miniz
    libressl
  ];

  patches = [ ];
})

