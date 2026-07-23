// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <vector>

#include "common/types.h"

namespace Vulkan {

std::vector<u32> GenerateClipDistanceShaderSpirv(std::span<std::tuple<u8, u8>> clip_locations);

} // namespace Vulkan
