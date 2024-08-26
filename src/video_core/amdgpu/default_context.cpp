// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/types.h"
#include "video_core/amdgpu/liverpool.h"

#include <array>

namespace AmdGpu {

// The following values are taken from fpPS4:
// https://github.com/red-prig/fpPS4/blob/436b43064be4c78229500f3d3c054fc76639247d/chip/pm4_pfp.pas#L410
//
static constexpr std::array reg_array_default{
    0x00000000u, 0x80000000u, 0x40004000u, 0xdeadbeefu, 0x00000000u, 0x40004000u, 0x00000000u,
    0x40004000u, 0x00000000u, 0x40004000u, 0x00000000u, 0x40004000u, 0xaa99aaaau, 0x00000000u,
    0xdeadbeefu, 0xdeadbeefu, 0x80000000u, 0x40004000u, 0x00000000u, 0x00000000u, 0x80000000u,
    0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u,
    0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u,
    0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u,
    0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u,
    0x40004000u, 0x80000000u, 0x40004000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u,
    0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u,
    0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u,
    0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u,
    0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u,
    0x2a00161au,
};

void Liverpool::Regs::SetDefaults() {
    std::memset(reg_array.data(), 0, reg_array.size() * sizeof(u32));

    std::memcpy(&reg_array[ContextRegWordOffset + 0x80], reg_array_default.data(),
                reg_array_default.size() * sizeof(u32));

    // Individual context regs values
    reg_array[ContextRegWordOffset + 0x000d] = 0x40004000u;
    reg_array[ContextRegWordOffset + 0x01b6] = 0x00000002u;
    reg_array[ContextRegWordOffset + 0x0204] = 0x00090000u;
    reg_array[ContextRegWordOffset + 0x0205] = 0x00000004u;
    reg_array[ContextRegWordOffset + 0x0295] = 0x00000100u;
    reg_array[ContextRegWordOffset + 0x0296] = 0x00000080u;
    reg_array[ContextRegWordOffset + 0x0297] = 0x00000002u;
    reg_array[ContextRegWordOffset + 0x02aa] = 0x00001000u;
    reg_array[ContextRegWordOffset + 0x02f7] = 0x00001000u;
    reg_array[ContextRegWordOffset + 0x02f9] = 0x00000005u;
    reg_array[ContextRegWordOffset + 0x02fa] = 0x3f800000u;
    reg_array[ContextRegWordOffset + 0x02fb] = 0x3f800000u;
    reg_array[ContextRegWordOffset + 0x02fc] = 0x3f800000u;
    reg_array[ContextRegWordOffset + 0x02fd] = 0x3f800000u;
    reg_array[ContextRegWordOffset + 0x0316] = 0x0000000eu;
    reg_array[ContextRegWordOffset + 0x0317] = 0x00000010u;
}

} // namespace AmdGpu
