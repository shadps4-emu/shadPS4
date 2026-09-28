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
            , autoPatchelfHook
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
            , vulkan-loader
            , libxkbcommon
            , systemdMinimal
            , pugixml
            , libuuid
            , libx11
            , releaseMode ? "RelWithDebInfo"
            , dontStrip ? true
            , enableDiscordRpc ? false
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
                autoPatchelfHook
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
              ] ++ x11Libs;

              cmakeFlags = [
                (lib.cmakeFeature "CMAKE_BUILD_TYPE" releaseMode)
                (lib.cmakeBool "ENABLE_DISCORD_RPC" enableDiscordRpc)
                (lib.cmakeBool "ENABLE_TESTS" false)
                (lib.cmakeBool "CMAKE_CXX_SCAN_FOR_MODULES" false)
                (lib.cmakeFeature "FETCHCONTENT_SOURCE_DIR_FMT" "${self}/externals/fmt")
              ];

              inherit dontStrip;

              patchPhase = '' 
                substituteInPlace externals/ffmpeg-core/CMakeLists.txt \
                  --replace-fail 'set(FFMPEG_ZIP_PATH "''${CMAKE_BINARY_DIR}/externals/ffmpeg-''${FFMPEG_GIT_SHA}.zip")' 'set(FFMPEG_ZIP_PATH "''${CMAKE_BINARY_DIR}/externals/ffmpeg-${ffmpegZip.commit}.zip")'
                
                substituteInPlace src/common/scm_rev.cpp.in \
                  --replace-fail "@GIT_BRANCH@" "${self.shortRev or "Dirty"}"
                
                substituteInPlace src/common/scm_rev.cpp.in \
                  --replace-fail "@GIT_DESC@" ""
              '';

              preConfigure = ''
                mkdir -p build/externals
                ln -sf ${ffmpegZip.path} build/externals/ffmpeg-${ffmpegZip.commit}.zip
                echo "linked ${ffmpegZip.path} to $(pwd ../build/externals)"
              '';

              runtimeDependencies = [
                vulkan-loader
              ];
              autoPatchelfIgnoreMissingDeps = [ "*" ];
            });

          defaultBuild = pkgsLinux.callPackage build { releaseMode = "relWithDebInfo"; dontStrip = true; };
        in
        {
          debug = pkgsLinux.callPackage build { releaseMode = "debug"; dontStrip = true; };
          release = pkgsLinux.callPackage build { releaseMode = "release"; dontStrip = false; };
          releaseWithDebInfo = (defaultBuild);
          default = (defaultBuild);
        };
    };
}
