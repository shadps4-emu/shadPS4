// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/bit_field.h"
#include "common/types.h"
#include "video_core/amdgpu/pixel_format.h"

namespace AmdGpu {

// Table 8.5 Buffer Resource Descriptor [Sea Islands Series Instruction Set Architecture]
struct Buffer {
    union {
        BitField<0, 44, u64> base_address;
        BitField<48, 14, u64> stride;
        BitField<62, 1, u64> cache_swizzle;
        BitField<63, 1, u64> swizzle_enable;
    };
    u32 num_records;
    union {
        BitField<0, 3, u32> dst_sel_x;
        BitField<3, 3, u32> dst_sel_y;
        BitField<6, 3, u32> dst_sel_z;
        BitField<9, 3, u32> dst_sel_w;
        BitField<12, 3, NumberFormat> num_format;
        BitField<15, 4, DataFormat> data_format;
        BitField<19, 2, u32> element_size;
        BitField<21, 2, u32> index_stride;
    };
};

} // namespace AmdGpu
