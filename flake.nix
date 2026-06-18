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
            { self
            , mkShell
            , clangStdenv
            , clang-tools
            , cmake
            , pkg-config
            , renderdoc
            , gef
            , strace
            , perf
            , sdl3
            , vulkan-tools
            , libGL
            , jack1
            , fribidi
            , libthai
            , libpulseaudio
            , sndio
            , libdrm
            , libgbm
            , libusb1
            , libxkbcommon
            , libxcursor
            , libxext
            , libxfixes
            , libxi
            , libxinerama
            , libxrandr
            , libxrender
            , libxtst
            , libxscrnsaver
            , enableDebugTooling ? true
            ,
            }:

            mkShell.override { stdenv = clangStdenv; } {
              inputsFrom = [ self.packages.x86_64-linux.default ];

              packages =
                let
                  # SDL3 requres extra libraries inside the devshell in order to pass CMake's configure.
                  sdlConfigureDeps = [
                    libGL
                    jack1
                    fribidi
                    libthai
                    libpulseaudio
                    sndio
                    libdrm
                    libgbm
                    libusb1
                    libxkbcommon
                    libxcursor
                    libxext
                    libxfixes
                    libxi
                    libxinerama
                    libxrandr
                    libxrender
                    libxtst
                    libxscrnsaver
                  ];
                in
                [
                  clang-tools
                  cmake
                  pkg-config
                ] ++ sdlConfigureDeps ++ pkgsLinux.lib.optionals enableDebugTooling [ renderdoc gef strace perf vulkan-tools ];

              shellHook = ''
                echo "Entering shadPS4 development shell!"
              '';

              CMAKE_C_COMPILER = "clang";
              CMAKE_CXX_COMPILER = "clang++";
              CMAKE_EXPORT_COMPILE_COMMANDS = "ON";
            };

        in
        pkgsLinux.callPackage shell { inherit self; };

      packages.x86_64-linux =
        let
          buildSettings = {
            "release" = { symbols = false; flag = "-DCMAKE_BUILD_TYPE=Release"; };
            "relWithDebInfo" = { symbols = true; flag = "-DCMAKE_BUILD_TYPE=RelWithDebInfo"; };
            "debug" = { symbols = true; flag = "-DCMAKE_BUILD_TYPE=Debug"; };
          };
          getBuildSettings = chosenBuild: buildSettings.${chosenBuild} or (abort "Build mode not valid! Use \"debug\", \"release\", or \"relWithDebInfo\".");

          build =
            { clangStdenv
            , cmake
            , ninja
            , pkg-config
            , libX11
            , libxrandr
            , libxext
            , libxcursor
            , libxi
            , libxscrnsaver
            , libxtst
            , libxcb
            , boost
            , cli11
            , ffmpeg
            , fmt
            , freetype
            , glslang
            , magic-enum
            , miniupnpc
            , miniz
            , nlohmann_json
            , libpng
            , openal
            , libressl
            , renderdoc
            , sdl3
            , stb
            , toml11
            , robin-map
            , vulkan-headers
            , vulkan-memory-allocator
            , xbyak
            , xxhash
            , zlib
            , zydis
            , pugixml
            , libuuid
            , systemdMinimal
            , libx11
            , releaseMode ? "debug"
            , enableDiscordRpc ? false
            ,
            }:

            clangStdenv.mkDerivation (finalAttrs: {
              name = "${finalAttrs.pname}-${finalAttrs.version}-${finalAttrs.system}";
              pname = "shadps4";
              version = "0.16.1";
              system = "x86_64-linux";
              src = ./.;

              nativeBuildInputs = [
                cmake
                ninja
                pkg-config
              ];
              buildInputs = [
                boost
                cli11
                ffmpeg
                fmt
                freetype
                glslang
                magic-enum
                miniupnpc
                miniz
                nlohmann_json
                libpng
                openal
                libressl
                renderdoc
                sdl3
                stb
                toml11
                robin-map
                vulkan-headers
                vulkan-memory-allocator
                xbyak
                xxhash
                zlib
                zydis
                pugixml
                libuuid
                systemdMinimal
                libx11
              ];

              cmakeFlags = [
                (getBuildSettings releaseMode).flag
                (pkgsLinux.lib.cmakeBool "ENABLE_DISCORD_RPC" enableDiscordRpc)
                (pkgsLinux.lib.cmakeBool "ENABLE_TESTS" false)
                (pkgsLinux.lib.cmakeBool "ENABLE_SYSTEM_LIBRARIES" true)
              ];
              dontStrip = (getBuildSettings releaseMode).symbols;

              # Cannot get the Branch name from the sandbox.
              # Getting the commit hash can still be acquired through self.
              patchPhase = '' 
                substituteInPlace src/common/scm_rev.cpp.in \
                  --replace-fail "@GIT_BRANCH@" "${self.shortRev or "Dirty"}"
                
                substituteInPlace src/common/scm_rev.cpp.in \
                  --replace-fail "@GIT_DESC@" ""
              '';
            });
        in
        {
          debug = pkgsLinux.callPackage build { releaseMode = "debug"; };
          release = pkgsLinux.callPackage build { releaseMode = "release"; };
          releaseWithDebInfo = pkgsLinux.callPackage build { releaseMode = "relWithDebInfo"; };
          default = pkgsLinux.callPackage build { releaseMode = "relWithDebInfo"; };
        };
    };
}
