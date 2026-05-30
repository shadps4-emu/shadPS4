## SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
## SPDX-License-Identifier: GPL-2.0-or-later

{
  rootPath,
  platform,
  buildFlags,
  useDebugSymbols ? true,
}:

pkgs.clangStdenv.stdenv.mkDerivation (finalAttrs: {
  pname = "shadps4";
  version = "0.15.1";
  system = platform;
  src = rootPath;

  nativeBuildInputs = [ ];
  buildInputs = [ ];
  cmakeFlags = buildFlags;
  dontStrip = debugSymbols;
  
  patches = [ ];
});

