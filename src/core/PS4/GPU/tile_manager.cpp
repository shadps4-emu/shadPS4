// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include "common/singleton.h"
#include "core/PS4/GPU/tile_manager.h"

namespace GPU {

static u32 IntLog2(u32 i) {
    return 31 - __builtin_clz(i | 1u);
}

class TileManager {
public:
    TileManager() {}
    virtual ~TileManager() {}
    std::mutex m_mutex;
};

class TileManager32 {
public:
    u32 m_macro_tile_height = 0;
    u32 m_bank_height = 0;
    u32 m_num_banks = 0;
    u32 m_num_pipes = 0;
    u32 m_padded_width = 0;
    u32 m_padded_height = 0;
    u32 m_pipe_bits = 0;
    u32 m_bank_bits = 0;

    void Init(u32 width, u32 height, bool is_neo) {
        m_macro_tile_height = (is_neo ? 128 : 64);
        m_bank_height = is_neo ? 2 : 1;
        m_num_banks = is_neo ? 8 : 16;
        m_num_pipes = is_neo ? 16 : 8;
        m_padded_width = width;
        if (height == 1080) {
            m_padded_height = is_neo ? 1152 : 1088;
        }
        if (height == 720) {
            m_padded_height = 768;
        }
        m_pipe_bits = is_neo ? 4 : 3;
        m_bank_bits = is_neo ? 3 : 4;
    }

    static u32 getElementIdx(u32 x, u32 y) {
        u32 elem = 0;
        elem |= ((x >> 0u) & 0x1u) << 0u;
        elem |= ((x >> 1u) & 0x1u) << 1u;
        elem |= ((y >> 0u) & 0x1u) << 2u;
        elem |= ((x >> 2u) & 0x1u) << 3u;
        elem |= ((y >> 1u) & 0x1u) << 4u;
        elem |= ((y >> 2u) & 0x1u) << 5u;

        return elem;
    }

    static u32 getPipeIdx(u32 x, u32 y, bool is_neo) {
        u32 pipe = 0;

        if (!is_neo) {
            pipe |= (((x >> 3u) ^ (y >> 3u) ^ (x >> 4u)) & 0x1u) << 0u;
            pipe |= (((x >> 4u) ^ (y >> 4u)) & 0x1u) << 1u;
            pipe |= (((x >> 5u) ^ (y >> 5u)) & 0x1u) << 2u;
        } else {
            pipe |= (((x >> 3u) ^ (y >> 3u) ^ (x >> 4u)) & 0x1u) << 0u;
            pipe |= (((x >> 4u) ^ (y >> 4u)) & 0x1u) << 1u;
            pipe |= (((x >> 5u) ^ (y >> 5u)) & 0x1u) << 2u;
            pipe |= (((x >> 6u) ^ (y >> 5u)) & 0x1u) << 3u;
        }

        return pipe;
    }

    static u32 getBankIdx(u32 x, u32 y, u32 bank_width, u32 bank_height, u32 num_banks,
                          u32 num_pipes) {
        const u32 x_shift_offset = IntLog2(bank_width * num_pipes);
        const u32 y_shift_offset = IntLog2(bank_height);
        const u32 xs = x >> x_shift_offset;
        const u32 ys = y >> y_shift_offset;
        u32 bank = 0;
        switch (num_banks) {
        case 8:
            bank |= (((xs >> 3u) ^ (ys >> 5u)) & 0x1u) << 0u;
            bank |= (((xs >> 4u) ^ (ys >> 4u) ^ (ys >> 5u)) & 0x1u) << 1u;
            bank |= (((xs >> 5u) ^ (ys >> 3u)) & 0x1u) << 2u;
            break;
        case 16:
            bank |= (((xs >> 3u) ^ (ys >> 6u)) & 0x1u) << 0u;
            bank |= (((xs >> 4u) ^ (ys >> 5u) ^ (ys >> 6u)) & 0x1u) << 1u;
            bank |= (((xs >> 5u) ^ (ys >> 4u)) & 0x1u) << 2u;
            bank |= (((xs >> 6u) ^ (ys >> 3u)) & 0x1u) << 3u;
            break;
        default:;
        }

        return bank;
    }

    u64 getTiledOffs(u32 x, u32 y, bool is_neo) const {
        u64 element_index = getElementIdx(x, y);

        u32 xh = x;
        u32 yh = y;
        u64 pipe = getPipeIdx(xh, yh, is_neo);
        u64 bank = getBankIdx(xh, yh, 1, m_bank_height, m_num_banks, m_num_pipes);
        u32 tile_bytes = (8 * 8 * 32 + 7) / 8;
        u64 element_offset = (element_index * 32);
        u64 tile_split_slice = 0;

        if (tile_bytes > 512) {
            tile_split_slice = element_offset / (static_cast<u64>(512) * 8);
            element_offset %= (static_cast<u64>(512) * 8);
            tile_bytes = 512;
        }

        u64 macro_tile_bytes =
            (128 / 8) * (m_macro_tile_height / 8) * tile_bytes / (m_num_pipes * m_num_banks);
        u64 macro_tiles_per_row = m_padded_width / 128;
        u64 macro_tile_row_index = y / m_macro_tile_height;
        u64 macro_tile_column_index = x / 128;
        u64 macro_tile_index =
            (macro_tile_row_index * macro_tiles_per_row) + macro_tile_column_index;
        u64 macro_tile_offset = macro_tile_index * macro_tile_bytes;
        u64 macro_tiles_per_slice = macro_tiles_per_row * (m_padded_height / m_macro_tile_height);
        u64 slice_bytes = macro_tiles_per_slice * macro_tile_bytes;
        u64 slice_offset = tile_split_slice * slice_bytes;
        u64 tile_row_index = (y / 8) % m_bank_height;
        u64 tile_index = tile_row_index;
        u64 tile_offset = tile_index * tile_bytes;

        u64 tile_split_slice_rotation = ((m_num_banks / 2) + 1) * tile_split_slice;
        bank ^= tile_split_slice_rotation;
        bank &= (m_num_banks - 1);

        u64 total_offset = (slice_offset + macro_tile_offset + tile_offset) * 8 + element_offset;
        u64 bit_offset = total_offset & 0x7u;
        total_offset /= 8;

        u64 pipe_interleave_offset = total_offset & 0xffu;
        u64 offset = total_offset >> 8u;
        u64 byte_offset = pipe_interleave_offset | (pipe << (8u)) | (bank << (8u + m_pipe_bits)) |
                          (offset << (8u + m_pipe_bits + m_bank_bits));

        return ((byte_offset << 3u) | bit_offset) / 8;
    }
};

void convertTileToLinear(void* dst, const void* src, u32 width, u32 height, bool is_neo) {
    TileManager32 t;
    t.Init(width, height, is_neo);

    auto* g_TileManager = Common::Singleton<TileManager>::Instance();

    std::scoped_lock lock{g_TileManager->m_mutex};

    for (u32 y = 0; y < height; y++) {
        u32 x = 0;
        u64 linear_offset = y * width * 4;

        for (; x + 1 < width; x += 2) {
            auto tiled_offset = t.getTiledOffs(x, y, is_neo);

            *reinterpret_cast<u64*>(static_cast<u8*>(dst) + linear_offset) =
                *reinterpret_cast<const u64*>(static_cast<const u8*>(src) + tiled_offset);
            linear_offset += 8;
        }
        if (x < width) {
            auto tiled_offset = t.getTiledOffs(x, y, is_neo);

            *reinterpret_cast<u32*>(static_cast<u8*>(dst) + linear_offset) =
                *reinterpret_cast<const u32*>(static_cast<const u8*>(src) + tiled_offset);
        }
    }
}
} // namespace GPU
