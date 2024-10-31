// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/container/set.hpp>
#include <boost/container/small_vector.hpp>
#include "common/types.h"

namespace Shader {

using PFN_SrtWalker = void PS4_SYSV_ABI (*)(const u32* /*user_data*/, u32* /*flat_dst*/);

struct PersistentSrtInfo {
    // Special case when fetch shader uses step rates.
    struct SrtSharpReservation {
        u32 sgpr_base;
        u32 dword_offset;
        u32 num_dwords;
    };

    PFN_SrtWalker walker_func{};
    boost::container::small_vector<SrtSharpReservation, 2> srt_reservations;
    u32 flattened_bufsize_dw = 16; // NumUserDataRegs

    // Special case for fetch shaders because we don't generate IR to read from step rate buffers,
    // so we won't see usage with GetUserData/ReadConst.
    // Reserve space in the flattened buffer for a sharp ahead of time
    u32 ReserveSharp(u32 sgpr_base, u32 dword_offset, u32 num_dwords) {
        u32 rv = flattened_bufsize_dw;
        srt_reservations.emplace_back(sgpr_base, dword_offset, num_dwords);
        flattened_bufsize_dw += num_dwords;
        return rv;
    }
};

} // namespace Shader