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
      formatter.x86_64-linux = pkgsLinux.nixpkgs-fmt;

      devShells.x86_64-linux.default = pkgsLinux.mkShell.override { stdenv = pkgsLinux.clangStdenv; } {
        packages = with pkgsLinux; [
          clang-tools
          cmake
          pkg-config
          vulkan-tools

          renderdoc
          gef
          strace
          perf

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

        CMAKE_C_COMPILER = "clang";
        CMAKE_CXX_COMPILER = "clang++";
        CMAKE_EXPORT_COMPILE_COMMANDS = "ON";
      };

      packages.x86_64-linux = let
        debugBuild = pkgsLinux.callPackage "${self}/nix/modules/build-shadps4.nix" 
        {
          src = "${self}";
          system = "x86_64-linux";
          cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Debug" ];
        };
        releaseBuild = pkgsLinux.callPackage "${self}/nix/modules/build-shadps4.nix" 
        {
          src = "${self}";
          system = "x86_64-linux";
          dontStrip = false;
          cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Release" ];
        };
        releaseWithDebugInfoBuild = pkgsLinux.callPackage "${self}/nix/modules/build-shadps4.nix" 
        {
          src = "${self}";
          system = "x86_64-linux";
          cmakeFlags = [ "-DCMAKE_BUILD_TYPE=RelWithDebInfo" ];
        };
      in 
        {
          debug = debugBuild;
          release = releaseBuild;
          releaseWithDebugInfo = releaseWithDebugInfoBuild;
          default = releaseWithDebugInfoBuild;
        };
    };
}
