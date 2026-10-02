// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace AmdGpu {

/// When set, every guest colour/depth buffer and multisampled texture is treated as
/// single-sampled by the host renderer (guest memory sizes are not affected).
///
/// God of War III Remastered renders its scene with 2x MSAA and then reads the
/// multisampled depth buffer as a texture. On some GPUs that read returns zeros in the
/// lower half of 8x8 depth tiles on flat surfaces, which shows up as horizontal stripes
/// in every effect that uses depth. The game applies its own post-process anti-aliasing,
/// so dropping MSAA on the host avoids the broken path at little visual cost.
inline bool g_force_no_msaa = false;

} // namespace AmdGpu
