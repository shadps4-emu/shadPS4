// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/object_pool.h"

namespace Shader {

struct BinaryInfo {
    u8 signature[7];
    u8 version;
    u32 pssl_or_cg : 1;
    u32 cached : 1;
    u32 type : 4;
    u32 source_type : 2;
    u32 length : 24;
    u8 chunk_usage_base_offset_in_dw;
    u8 num_input_usage_slots;
    u8 is_srt : 1;
    u8 is_srt_used_info_valid : 1;
    u8 is_extended_usage_info : 1;
    u8 reserved2 : 5;
    u8 reserved3;
    u64 shader_hash;
    u32 crc32;
};

[[nodiscard]] IR::Program TranslateProgram(ObjectPool<IR::Inst>& inst_pool,
                                           ObjectPool<IR::Block>& block_pool,
                                           std::span<const u32> code, const Info&& info);

} // namespace Shader
