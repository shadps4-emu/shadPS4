// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include "video_core/amdgpu/pixel_format.h"

namespace AmdGpu {

u32 getNumComponents(DataFormat format) {
    constexpr std::array numComponentsPerElement = {
        0,  1,  1,  2,  1,  2,  3,  3,  4,  4,  4,  2, 4, 3, 4, -1, 3, 4, 4, 4, 2,
        2,  2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 3, 3, 4, 4,  4, 1, 2, 3, 4,
        -1, -1, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 2,  3, 1, 1};

    const u32 index = static_cast<u32>(format);
    if (index >= numComponentsPerElement.size()) {
        return 0;
    }
    return numComponentsPerElement[index];
}

} // namespace AmdGpu
