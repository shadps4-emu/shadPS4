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

      devShells.x86_64-linux.default = pkgsLinux.callPackage "${self}/nix/shell.nix" {};
      
      packages.x86_64-linux = let
        debugBuild = pkgsLinux.callPackage "${self}/nix/build.nix" 
        {
          src = "${self}";
          system = "x86_64-linux";
          cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Debug" ];
        };
        releaseBuild = pkgsLinux.callPackage "${self}/nix/build.nix" 
        {
          src = "${self}";
          system = "x86_64-linux";
          dontStrip = false;
          cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Release" ];
        };
        releaseWithDebugInfoBuild = pkgsLinux.callPackage "${self}/nix/build.nix" 
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
