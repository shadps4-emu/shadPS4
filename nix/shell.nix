{
  mkShell,
  clangStdenv,

  clang-tools,
  cmake,
  pkg-config,
  vulkan-tools,

  renderdoc,
  gef,
  strace,
  perf,

  openal,
  zlib,
  libedit,
  vulkan-headers,
  vulkan-utility-libraries,
  ffmpeg,
  fmt,
  glslang,
  wayland,
  stb,
  libpng,
  libuuid,

  sdl3,
  alsa-lib,
  hidapi,
  ibus,
  jack2,
  libdecor,
  libthai,
  fribidi,
  libxcb,
  libGL,
  libpulseaudio,
  libusb1,
  libx11,
  libxcursor,
  libxext,
  libxfixes,
  libxi,
  libxinerama,
  libxkbcommon,
  libxrandr,
  libxrender,
  libxtst,
  pipewire,
  libxscrnsaver,
  sndio,
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
}

