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
        shellHook = ''
          echo "Entering shadPS4 development shell!"
        '';
      };

      linux =
        let
          execName = "shadps4";
          nativeInputs = with pkgsLinux; [
            cmake
            ninja
            pkg-config
            magic-enum
            fmt
            eudev
          ];
          buildInputs = with pkgsLinux; [
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
            vulkan-loader
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
          ];

          defaultFlags = [
            "-DCMAKE_INSTALL_PREFIX=$out"
          ];
        in
        {
          debug = pkgsLinux.stdenv.mkDerivation {
            pname = "${execName}";
            version = "git";
            system = "x86_64-linux";
            src = ./.;
            dontStrip = true;

            nativeBuildInputs = nativeInputs;
            buildInputs = buildInputs;
            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Debug"
            ] ++ [defaultFlags];
          };
          release = pkgsLinux.stdenv.mkDerivation {
            pname = "${execName}";
            version = "git";
            system = "x86_64-linux";
            src = ./.;

            nativeBuildInputs = nativeInputs;
            buildInputs = buildInputs;
            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
            ] ++ [defaultFlags];
          };
          releaseWithDebugInfo = pkgsLinux.stdenv.mkDerivation {
            pname = "${execName}";
            version = "git";
            system = "x86_64-linux";
            src = ./.;
            dontStrip = true;

            nativeBuildInputs = nativeInputs;
            buildInputs = buildInputs;
            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
            ] ++ [defaultFlags];
          };
        };
    };
}
