// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include "common/types.h"

namespace Shader {

/// Written by the GPU helper for prepared indirect dispatches. The host grid is read from command.
/// When the guest dimensions exceed the device limits, mode is 1 and the host dispatch is a
/// reshaped grid of at least total (at most UINT32_MAX) workgroups, whose row count fits in 32
/// bits; compute shaders rebuild the guest workgroup id and skip the padding. Direct shaders
/// do not declare or bind this buffer.
struct IndirectDispatchMetadata {
    std::array<u32, 3> guest{};
    u32 status{}; ///< 0 when the dispatch is supported
    std::array<u32, 3> command{};
    u32 total{};
    u32 mode{};
    std::array<u32, 3> reserved{};
};
static_assert(sizeof(IndirectDispatchMetadata) == 48);

} // namespace Shader
