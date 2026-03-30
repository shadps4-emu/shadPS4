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
      
      debugLinux = 
      let
        exec_name = "shadps4";
      in 
      pkgsLinux.stdenv.mkDerivation {
        pname = "${exec_name}";
        version = "git";
        system = "x86_64-linux";
        src = ./.;
        dontStrip = true;

        nativeBuildInputs = with pkgsLinux; [ 
          cmake 
          ninja
          pkg-config
          #libcxx
          magic-enum
          fmt
          eudev
          makeWrapper
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
          libdecor
          libxkbcommon
          libGL
          #mesa
          #util-linux
          libuuid
          #libedit
          #sdl3
          #alsa-lib
          #libusb1
          #libgbm
          #ibusMinimal
          #libdrm
          #jack2
          #sndio
        ];

        cmakeFlags = [
          "-DCMAKE_BUILD_TYPE=Debug"
          "-DCMAKE_INSTALL_PREFIX=$out"
        ];

        postFixup = 
        let
          libs = with pkgsLinux; [
            libGL.out
            vulkan-loader.out
          ];
        in
        ''
          wrapProgram $out/bin/${exec_name} \
            --set LD_LIBRARY_PATH ${pkgsLinux.lib.makeLibraryPath libs} \
        '';

        #installPhase = ''
        #  runHook preInstall
        #  mkdir -p bin
        #  cp shadps4 $out/bin/
        #  runHook postInstall
        #'';
      };
    };
}
