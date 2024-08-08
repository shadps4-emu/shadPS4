<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

### Install Vulkan SDK and \*ensure `spirv-cross` and `glslc` are in PATH\*.

1. Enable `dumpShaders` in config.toml

2. Run `spirv-cross -V fs_0x000000.spv --output fs_0x000000.glsl` to decompile the SPIR-V IR to GLSL.

3. Edit the GLSL file as you wish

4. To compile back to SPIR-V, run (change the _**-fshader-stage**_ to correct stage):
   `glslc --target-env=vulkan1.3 --target-spv=spv1.6 -fshader-stage=frag fs_0x000000.glsl -o fs_0x000000.spv`

5. Put the updated .spv file to `shader/patch` folder with the same name as the original shader

6. Enable `patchShaders` in config.toml