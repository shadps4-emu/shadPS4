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

      devShells.x86_64-linux.default =
        let
          shell =
            { mkShell
            , clangStdenv
            , clang-tools
            , cmake
            , pkg-config
            , vulkan-tools
            , renderdoc
            , gef
            , strace
            , perf
            , openal
            , zlib
            , libedit
            , vulkan-headers
            , vulkan-utility-libraries
            , ffmpeg
            , fmt
            , glslang
            , wayland
            , stb
            , libpng
            , libuuid
            , sdl3
            , alsa-lib
            , hidapi
            , ibus
            , jack2
            , libdecor
            , libthai
            , fribidi
            , libxcb
            , libGL
            , libpulseaudio
            , libusb1
            , libx11
            , libxcursor
            , libxext
            , libxfixes
            , libxi
            , libxinerama
            , libxkbcommon
            , libxrandr
            , libxrender
            , libxtst
            , pipewire
            , libxscrnsaver
            , sndio
            , cli11
            , nlohmann_json
            , spdlog
            ,
            }:

            mkShell.override { stdenv = clangStdenv; } {
              packages = [
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
                cli11
                nlohmann_json
                spdlog.dev

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

        in
        pkgsLinux.callPackage shell { };

      packages.x86_64-linux =
        let
          build =
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
              version = "0.16.1";

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

              # Cannot get the Branch name from the sandbox.
              # Getting the commit hash can still be acquired through self.
              patchPhase = '' 
                substituteInPlace src/common/scm_rev.cpp.in \
                  --replace-fail "@GIT_BRANCH@" "${self.shortRev or "Dirty"}"
                
                substituteInPlace src/common/scm_rev.cpp.in \
                  --replace-fail "@GIT_DESC@" ""
              '';
            });

          debugBuild = pkgsLinux.callPackage build
            {
              src = "${self}";
              system = "x86_64-linux";
              cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Debug" ];
            };
          releaseBuild = pkgsLinux.callPackage build
            {
              src = "${self}";
              system = "x86_64-linux";
              dontStrip = false;
              cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Release" ];
            };
          releaseWithDebugInfoBuild = pkgsLinux.callPackage build
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
