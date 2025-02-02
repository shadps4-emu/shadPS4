### Vulkan packages in official nix repo are still using version 1.3.296.0 while 1.4.305 or 1.4.304.0 is required for this package, so we have to use a custom vulkan packages to build shadPS4.

with import (fetchTarball "https://github.com/NixOS/nixpkgs/archive/606996d74f6e2a12635d41c1bf58bfc7ea3bb5ec.tar.gz") { };

let
  vulkanLoaderCustom = pkgs.stdenv.mkDerivation rec {
    pname = "vulkan-loader";
    version = "1.4.305";

    src = fetchFromGitHub {
      owner = "KhronosGroup";
      repo = "Vulkan-Loader";
      rev = "v${version}";
      hash = "sha256-nAxb2WH4FUNyZ0daO1lEcuuIbqLTjNDsc95m3NIY8F8=";
    };

    nativeBuildInputs = [ cmake pkg-config ];
    buildInputs = [ vulkanHeadersCustom ]
      ++ lib.optionals stdenv.hostPlatform.isLinux [ xorg.libX11 xorg.libxcb xorg.libXrandr wayland ];

    cmakeFlags = [ "-DCMAKE_INSTALL_INCLUDEDIR=${vulkanHeadersCustom}/include" ]
      ++ lib.optional stdenv.hostPlatform.isDarwin "-DSYSCONFDIR=${moltenvk}/share"
      ++ lib.optional stdenv.hostPlatform.isLinux "-DSYSCONFDIR=${addDriverRunpath.driverLink}/share"
      ++ lib.optional (stdenv.buildPlatform != stdenv.hostPlatform) "-DUSE_GAS=OFF";

    outputs = [ "out" "dev" ];

    doInstallCheck = true;

    installCheckPhase = ''
      vulkan_path="${vulkanHeadersCustom}/include"
      stripped_path="''${vulkan_path#/nix/store/}"
      grep -q "$stripped_path" $dev/lib/pkgconfig/vulkan.pc || {
        echo vulkanHeadersCustom include directory not found in pkg-config file
        exit 1
      }
    '';

    passthru = {
      tests.pkg-config = testers.hasPkgConfigModules {
        package = finalAttrs.finalPackage;
      };
    };
  };

  vulkanUtilsCustom = pkgs.stdenv.mkDerivation rec {
    pname = "vulkan-utility-libraries";
    version = "1.4.305";

    src = fetchFromGitHub {
      owner = "KhronosGroup";
      repo = "Vulkan-Utility-Libraries";
      rev = "vulkan-sdk-${version}";
      hash = "sha256-YBket/4gsAkkr1eTQXz8lXGyQHtY5mm8jLPKAqSaawM=";
    };

    nativeBuildInputs = [
      cmake
      python3
    ];
    buildInputs = [ vulkanHeadersCustom ];
  };
  vulkanHeadersCustom = pkgs.stdenv.mkDerivation rec {
    pname = "vulkan-headers";
    version = "1.4.305";
    src = pkgs.fetchFromGitHub {
      owner = "KhronosGroup";
      repo = "Vulkan-Headers";
      rev = "v${version}";
      hash = "sha256-r5tgUxu+ZGzxBGAfLxX1bW4YshRdqCwxVQJsoQrtY/Y=";
    };

    cmakeFlags = lib.optionals stdenv.hostPlatform.isDarwin [ "-DVULKAN_HEADERS_ENABLE_MODULE=OFF" ];

    nativeBuildInputs = [
      pkgs.cmake
      pkgs.ninja
    ];
  };

  llvmPackages = pkgs.llvmPackages_18;

  overlay = self: super: {
    vulkan-headers = vulkanHeadersCustom;
    vulkan-utility-libraries = vulkanUtilsCustom;
    vulkan-loader = vulkanLoaderCustom;
  };
in

with import (fetchTarball "https://github.com/NixOS/nixpkgs/archive/606996d74f6e2a12635d41c1bf58bfc7ea3bb5ec.tar.gz") { overlays = [ overlay ]; };

llvmPackages.stdenv.mkDerivation {
  name = "shadps4-build-env";
  nativeBuildInputs = [
    clang
    cmake
    pkg-config
    git
  ];
  buildInputs = [
    alsa-lib
    libpulseaudio
    openal
    openssl
    zlib
    libedit
    udev
    libevdev
    SDL2
    jack2
    sndio
    qt6.qtbase
    qt6.qttools
    qt6.qtmultimedia
    vulkanHeadersCustom
    vulkanUtilsCustom
    ffmpeg
    fmt
    glslang
    libxkbcommon
    wayland
    xorg.libxcb
    xorg.xcbutil
    xorg.xcbutilkeysyms
    xorg.xcbutilwm
    sdl3
    stb
    qt6.qtwayland
    wayland
    wayland-protocols
    libxkbcommon
  ];
  shellHook = ''
    export QT_QPA_PLATFORM="wayland"
    export QT_PLUGIN_PATH="${qt6.qtwayland}/lib/qt-6/plugins:${qt6.qtbase}/lib/qt-6/plugins"
    export QML2_IMPORT_PATH="${qt6.qtbase}/lib/qt-6/qml"
    export CMAKE_PREFIX_PATH="${vulkanHeadersCustom}:$CMAKE_PREFIX_PATH"

    # NVIDIA/OpenGL
    export LD_LIBRARY_PATH="${lib.makeLibraryPath [ libglvnd ]}:$LD_LIBRARY_PATH"

    export QT_PLUGIN_PATH="${qt6.qtbase}/lib/qt-6/plugins"
    export QT_QPA_PLATFORM="xcb"
    export LDFLAGS="-L${llvmPackages.libcxx}/lib -lc++"
    export LC_ALL="C.UTF-8"
    export XAUTHORITY=${builtins.getEnv "XAUTHORITY"}
  '';

}
