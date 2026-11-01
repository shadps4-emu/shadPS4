// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>
#include <span>
#include <vector>
#include "common/types.h"

namespace Shader {

enum class ShaderInputUsageType : u8 {
    ImmShaderResourceTable = 0x0A,
};

/**
 * SDK-compatible Gnm::InputUsageSlot. The shader binary footer stores this table immediately
 * before the chunk usage masks referenced by BinaryInfo::chunk_usage_base_offset_in_dw.
 */
struct alignas(4) ShaderInputUsageSlot {
    u8 usage_type;
    u8 api_slot;
    u8 start_register;
    u8 flags;

    [[nodiscard]] constexpr ShaderInputUsageType UsageType() const noexcept {
        return static_cast<ShaderInputUsageType>(usage_type);
    }
};
static_assert(sizeof(ShaderInputUsageSlot) == 4);

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

    struct SrtPointer {
        u32 size_in_dwords;
        std::span<const u8> layout;
    };

    struct SrtElement {
        u32 size_in_bytes;
        u32 metadata_bytes;
    };

    static std::optional<SrtElement> ReadSrtElement(std::span<const u8> layout) {
        if (layout.empty()) {
            return std::nullopt;
        }
        const u8 type = layout.front();
        if (type == 0x82 || type == 0xca || type == 0xda || type == 0xea || type == 0xfa) {
            return layout.size() >= 9 ? std::optional{SrtElement{8, 9}} : std::nullopt;
        }
        if (type == 0x0c) {
            return SrtElement{16, 1};
        }
        if ((type & 0xf) == 0xf) {
            return SrtElement{32, 1};
        }
        if ((type & 0xf) != 1 && (type & 0xf) != 9) {
            return std::nullopt;
        }
        const u32 size_code = type >> 4;
        if (size_code <= 6) {
            return SrtElement{1u << size_code, 1};
        }
        if (size_code == 7) {
            return SrtElement{12, 1};
        }
        if (size_code == 8 && layout.size() >= 3) {
            return SrtElement{(u32(layout[1]) | u32(layout[2]) << 8) + 1, 3};
        }
        return std::nullopt;
    }

    static std::optional<SrtPointer> FindSrtPointer(std::span<const u8> layout, u32 first_dword) {
        const u64 target = u64(first_dword) * sizeof(u32);
        u64 byte_offset{};
        for (size_t pos = 0; pos < layout.size() && byte_offset <= target;) {
            const u8 type = layout[pos];
            if (type == 0xca || type == 0xda || type == 0xea || type == 0xfa) {
                if (layout.size() - pos < 9) {
                    return std::nullopt;
                }
                const auto read_u32 = [&](size_t offset) {
                    return u32(layout[offset]) | (u32(layout[offset + 1]) << 8) |
                           (u32(layout[offset + 2]) << 16) | (u32(layout[offset + 3]) << 24);
                };
                if (byte_offset == target) {
                    const u32 offset = read_u32(pos + 1);
                    const u32 dwords = read_u32(pos + 5);
                    if (dwords == 0 || offset < 9 || offset >= layout.size() - pos) {
                        return std::nullopt;
                    }
                    return SrtPointer{dwords, layout.subspan(pos + offset)};
                }
            }
            const auto element = ReadSrtElement(layout.subspan(pos));
            if (!element) {
                return std::nullopt;
            }
            byte_offset += element->size_in_bytes;
            pos += element->metadata_bytes;
        }
        return std::nullopt;
    }

    std::optional<SrtPointer> FindSrtPointer(u32 user_data_reg,
                                             std::span<const u32> pointer_offsets = {}) const {
        if (user_data_reg < srt_user_data_base) {
            return std::nullopt;
        }
        auto pointer = FindSrtPointer(srt_metadata, user_data_reg - srt_user_data_base);
        for (const u32 offset : pointer_offsets) {
            if (!pointer || pointer->size_in_dwords < 2 || offset > pointer->size_in_dwords - 2) {
                return std::nullopt;
            }
            pointer = FindSrtPointer(pointer->layout, offset);
        }
        return pointer;
    }

    std::optional<u32> ImageArraySize(u32 user_data_reg, u32 first_dword = 0) const {
        const auto pointer = FindSrtPointer(user_data_reg);
        if (!pointer || first_dword >= pointer->size_in_dwords) {
            return std::nullopt;
        }
        u64 byte_offset{};
        const u64 target = u64(first_dword) * sizeof(u32);
        for (size_t pos = 0; pos < pointer->layout.size() && byte_offset <= target;) {
            const u8 type = pointer->layout[pos];
            if (byte_offset == target && (type & 0xf) == 0xf) {
                u32 count{};
                while (count < pointer->layout.size() - pos &&
                       pointer->layout[pos + count] == type) {
                    ++count;
                }
                if (count == 0 || count > (pointer->size_in_dwords - first_dword) / 8) {
                    return std::nullopt;
                }
                return count;
            }
            const auto element = ReadSrtElement(pointer->layout.subspan(pos));
            if (!element ||
                byte_offset + element->size_in_bytes > u64(pointer->size_in_dwords) * sizeof(u32)) {
                return std::nullopt;
            }
            byte_offset += element->size_in_bytes;
            pos += element->metadata_bytes;
        }
        return std::nullopt;
    }

    VAddr Base() const noexcept {
        return reinterpret_cast<VAddr>(code.data());
    }
};

} // namespace Shader
