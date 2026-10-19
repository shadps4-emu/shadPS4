// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <optional>
#include <span>
#include <vector>
#include "common/types.h"

namespace Shader {

/**
 * Compilation parameters used to identify and locate a guest shader program.
 */
struct ShaderParams {
    static constexpr u32 NumShaderUserData = 16;

    std::span<const u32, NumShaderUserData> user_data;
    std::span<const u32> code;
    u64 hash;
    std::vector<u8> srt_metadata;
    u32 srt_user_data_base{};

    std::optional<u32> ImageArraySize(u32 user_data_reg) const {
        if (user_data_reg < srt_user_data_base) {
            return std::nullopt;
        }
        const u32 target = user_data_reg - srt_user_data_base;
        u32 reg{};
        for (size_t pos = 0; pos < srt_metadata.size();) {
            const u8 type = srt_metadata[pos];
            if (type == 0xca) {
                if (srt_metadata.size() - pos < 9) {
                    return std::nullopt;
                }
                const auto read_u32 = [&](size_t offset) {
                    return u32(srt_metadata[offset]) | (u32(srt_metadata[offset + 1]) << 8) |
                           (u32(srt_metadata[offset + 2]) << 16) |
                           (u32(srt_metadata[offset + 3]) << 24);
                };
                if (reg == target) {
                    const u32 offset = read_u32(pos + 1);
                    const u32 dwords = read_u32(pos + 5);
                    if (dwords == 0 || dwords % 8 != 0 || offset >= srt_metadata.size() - pos) {
                        return std::nullopt;
                    }
                    const u32 count = dwords / 8;
                    const auto elements = std::span{srt_metadata}.subspan(pos + offset);
                    if (count >= elements.size() || elements[count] != 0 ||
                        !std::ranges::all_of(elements.first(count),
                                             [](u8 element) { return element == 0x1f; })) {
                        return std::nullopt;
                    }
                    return count;
                }
                reg += 2;
                pos += 9;
            } else if (type == 0x0c) {
                reg += 4;
                ++pos;
            } else if (type == 0x1f) {
                reg += 8;
                ++pos;
            } else {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

    VAddr Base() const noexcept {
        return reinterpret_cast<VAddr>(code.data());
    }
};

} // namespace Shader
