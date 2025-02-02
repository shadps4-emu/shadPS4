<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

## Why do we need custom packages?

Because vulkan packages in official nix repo are still using version 1.3.296.0 while 1.4.305 or 1.4.304.0 is required for this package - we have to use a custom nix-shell to build shadPS4.

You can also use stagging branch, but in that case all packages would have to be rebuild and that takes hours, to mitigate this issue we're only using custom vulkan packages which don't have too many dependencies.

## When will we be able to use packages from official repo?

Once https://github.com/NixOS/nixpkgs/pull/373969 will reach unstable branch we should be able to use packages from nixpkgs repo.
