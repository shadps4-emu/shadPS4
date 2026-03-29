## SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
## SPDX-License-Identifier: GPL-2.0-or-later

{
  description = "shadPS4 Nix Flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      pkgsLinux = nixpkgs.legacyPackages.x86_64-linux;
    in
    {
      devShells.x86_64-linux.default = pkgsLinux.mkShell.override { stdenv = pkgsLinux.clangStdenv; } {
        packages = with pkgsLinux; [
          clang-tools
          cmake
          pkg-config
          vulkan-tools

          renderdoc
          gef
          strace

          openal
          zlib.dev
          libedit.dev
          vulkan-headers
          vulkan-utility-libraries
          ffmpeg.dev
          fmt.dev
          glslang.dev
          wayland.dev
          stb
          libpng.dev
          libuuid

          # Specific SDL3 dependencies:
          sdl3.dev
          alsa-lib
          hidapi
          ibus.dev
          jack2.dev
          libdecor.dev
          libthai.dev
          fribidi.dev
          libxcb.dev
          libGL.dev
          libpulseaudio.dev
          libusb1.dev
          libx11.dev
          libxcursor.dev
          libxext
          libxfixes.dev
          libxi.dev
          libxinerama.dev
          libxkbcommon
          libxrandr.dev
          libxrender.dev
          libxtst
          pipewire.dev
          libxscrnsaver
          sndio
        ];

        LD_LIBRARY_PATH = pkgsLinux.lib.makeLibraryPath [
          pkgsLinux.mesa
        ];

        shellHook = ''
          echo "Entering shadPS4 development shell!"
        '';
      };
    };
}
