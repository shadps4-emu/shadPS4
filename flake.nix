{
  description = "shadPS4 Nix Flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = { self, nixpkgs }: 
  let
    pkgsLinux = nixpkgs.legacyPackages.x86_64-linux;
  in 
  {
    devShells.x86_64-linux.default = pkgsLinux.mkShell.override {stdenv = pkgsLinux.clangStdenv; } {
      packages = with pkgsLinux; [
        clang-tools
        cmake
        pkg-config
        vulkan-tools
        
        renderdoc
        gef
        strace

        sdl3.dev
        openal
        zlib.dev
        libedit.dev
        vulkan-headers
        vulkan-utility-libraries
        ffmpeg.dev
        fmt.dev
        glslang.dev
        libxkbcommon
        wayland.dev
        libxcb.dev
        stb
        libpng.dev
      ];

      shellHook = ''
        echo "Entering shadPS4 development shell!"
      '';
    };
  };
}
