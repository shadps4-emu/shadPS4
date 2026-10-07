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
            , lib
            , mkShell
            , clangStdenv
            , clang-tools
            , cmake
            , pkg-config
            , vulkan-loader
            , renderdoc
            , gef
            , strace
            , perf
            , vulkan-tools
            , libxkbcommon
            , libpulseaudio
            , wayland
            , libGL
            , enableDebugTooling ? true
            }:
            let
              runtimeBinaries = [
                libpulseaudio
                wayland
                vulkan-loader
                libGL
                libxkbcommon
              ];
            in
            mkShell.override { stdenv = clangStdenv; } {
              inputsFrom = [ self.packages.x86_64-linux.default ];

              packages = [
                clang-tools
                cmake
                pkg-config
              ] ++ lib.optionals enableDebugTooling [ renderdoc gef strace perf vulkan-tools ];

              shellHook = ''
                echo "Entering shadPS4 development shell!"
                export LD_LIBRARY_PATH="${lib.makeLibraryPath runtimeBinaries}:$LD_LIBRARY_PATH"
              '';

              CMAKE_C_COMPILER = "clang";
              CMAKE_CXX_COMPILER = "clang++";
              CMAKE_EXPORT_COMPILE_COMMANDS = "ON";
            };

        in
        pkgsLinux.callPackage shell { inherit self; };

      packages.x86_64-linux =
        let
          build =
            { clangStdenv
            , lib
            , fetchurl
            , cmake
            , ninja
            , python3
            , pkg-config
            , libX11
            , libxrandr
            , libxext
            , libxcursor
            , libxi
            , libxscrnsaver
            , libxtst
            , libxcb
            , libxfixes
            , libxinerama
            , libxrender
            , libGL
            , libdrm
            , libgbm
            , libpulseaudio
            , wayland
            , wayland-protocols
            , wayland-scanner
            , libxkbcommon
            , systemdMinimal
            , libuuid
            , libx11
            , sdl3
              # System Libraries:
            , boost
            , cli11
            , ffmpeg
            , fmt
            , freetype
            , glslang
            , half
            , magic-enum
            , miniupnpc
            , miniz
            , nlohmann_json
            , openal
            , libressl
            , renderdoc
            , stb
            , toml11
            , robin-map
            , vulkan-headers
            , vulkan-memory-allocator
            , vulkan-loader
            , xbyak
            , xxhash
            , zarchive
            , zlib
            , zydis
            , pugixml
              # Parameters:
            , dontStrip
            , releaseMode
            , enableDiscordRpc ? false
            , enableSystemLibraries ? false
            ,
            }:
            let
              getFfmpegZip = commit: hash: {
                inherit commit;
                path = fetchurl {
                  url = "https://github.com/shadps4-emu/ext-ffmpeg-core/releases/download/${commit}/ffmpeg-linux-x64.zip";
                  hash = "${hash}";
                };
              };
              ffmpegZip = (getFfmpegZip "94dde08" "sha256-qsu/uOYitoS8XTtM1sn5939d72SujYPAxbPr5leqM90=");

              x11Libs = [
                libx11
                libxcursor
                libxfixes
                libxi
                libxinerama
                libxrandr
                libxrender
                libxtst
                libxscrnsaver
                libxcb
              ];
            in
            clangStdenv.mkDerivation (finalAttrs: {
              name = "${finalAttrs.pname}-${finalAttrs.version}-${finalAttrs.system}";
              pname = "shadps4";
              version = "0.18.1";
              system = "${clangStdenv.hostPlatform.system}";
              src = ./.;

              nativeBuildInputs = [
                cmake
                ninja
                pkg-config
                python3
                wayland-scanner
              ];
              buildInputs = [
                libuuid
                wayland
                wayland-protocols
                libxkbcommon
                systemdMinimal
                libGL
                libxext
                libdrm
                libgbm
                libpulseaudio
                vulkan-loader
              ] ++ x11Libs
              ++ lib.optionals enableSystemLibraries [
                boost
                cli11
                ffmpeg
                fmt
                freetype
                glslang
                half
                magic-enum
                miniupnpc
                miniz
                nlohmann_json
                openal
                libressl
                renderdoc
                stb
                toml11
                robin-map
                vulkan-headers
                vulkan-memory-allocator
                xbyak
                xxhash
                zarchive
                zlib
                zydis
                pugixml
                sdl3
              ];

              cmakeFlags = [
                (lib.cmakeFeature "CMAKE_BUILD_TYPE" releaseMode)
                (lib.cmakeBool "ENABLE_DISCORD_RPC" enableDiscordRpc)
                (lib.cmakeBool "ENABLE_SYSTEM_LIBRARIES" enableSystemLibraries)
                (lib.cmakeBool "ENABLE_TESTS" false)
                (lib.cmakeBool "SDL_VULKAN" true)
                (lib.cmakeBool "SDL_WAYLAND" true)
                (lib.cmakeBool "SDL_X11" true)
              ] ++ lib.optionals (!enableSystemLibraries) [
                (lib.cmakeFeature "FETCHCONTENT_SOURCE_DIR_FMT" "${self}/externals/fmt")
                (lib.cmakeBool "CMAKE_CXX_SCAN_FOR_MODULES" false)
              ];

              inherit dontStrip;

              postPatch = ''
                # Pevents GIT-NOTFOUND in titlebar.
                substituteInPlace src/common/scm_rev.cpp.in \
                  --replace-fail '@GIT_BRANCH@' '${self.shortRev or "Dirty"}'
                substituteInPlace src/common/scm_rev.cpp.in \
                  --replace-fail '@GIT_DESC@' ' '
              ''
              + lib.optionalString (!enableSystemLibraries) ''
                # Pass the commit sha to the ffmpeg-core pulled before the build.
                substituteInPlace externals/ffmpeg-core/CMakeLists.txt \
                  --replace-fail \
                  'set(FFMPEG_ZIP_PATH "''${CMAKE_BINARY_DIR}/externals/ffmpeg-''${FFMPEG_GIT_SHA}.zip")' \
                  'set(FFMPEG_ZIP_PATH "''${CMAKE_BINARY_DIR}/externals/ffmpeg-${ffmpegZip.commit}.zip")'
                
                # SDL3 calls dlopen for libvulkan.so, replace with the Nix Path.
                substituteInPlace externals/sdl3/src/video/wayland/SDL_waylandvulkan.c \
                                  externals/sdl3/src/video/x11/SDL_x11vulkan.c \
                                  externals/sdl3/src/video/offscreen/SDL_offscreenvulkan.c \
                --replace-fail 'libvulkan.so' '${lib.getLib vulkan-loader}/lib/libvulkan.so'
              '';

              preConfigure = lib.optionalString (!enableSystemLibraries) ''
                mkdir -p build/externals
                ln -sf ${ffmpegZip.path} build/externals/ffmpeg-${ffmpegZip.commit}.zip
              '';
            });

          defaultBuild = pkgsLinux.callPackage build { releaseMode = "RelWithDebInfo"; dontStrip = true; };
        in
        {
          debug = pkgsLinux.callPackage build { releaseMode = "Debug"; dontStrip = true; };
          release = pkgsLinux.callPackage build { releaseMode = "Release"; dontStrip = false; };
          releaseWithDebInfo = (defaultBuild);
          default = (defaultBuild);
        };
    };
}
