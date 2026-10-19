// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include "common/types.h"

namespace Shader {

struct IndirectDispatchMetadata {
    std::array<u32, 3> guest{};
    u32 status{};
    std::array<u32, 3> command{};
    u32 total{};
    u32 mode{};
    std::array<u32, 3> reserved{};
};
static_assert(sizeof(IndirectDispatchMetadata) == 48);

struct ComputeDispatchData {
    static constexpr u32 Offset = 80;
    std::array<u32, 4> base{};
    std::array<u32, 4> count{};
};
static_assert(ComputeDispatchData::Offset + sizeof(ComputeDispatchData) <= 128);

template <typename Callback>
void ForEachDispatchChunk(const std::array<u32, 3>& count, const std::array<u32, 3>& limits,
                          Callback&& callback) {
    for (u32 z = 0; z < count[2];) {
        const u32 nz = std::min(count[2] - z, limits[2]);
        for (u32 y = 0; y < count[1];) {
            const u32 ny = std::min(count[1] - y, limits[1]);
            for (u32 x = 0; x < count[0];) {
                const u32 nx = std::min(count[0] - x, limits[0]);
                callback(std::array<u32, 3>{x, y, z}, std::array<u32, 3>{nx, ny, nz});
                x += nx;
            }
            y += ny;
        }
        z += nz;
    }
}
} // namespace Shader
