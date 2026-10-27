// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::VdecCore {

enum class OrbisVdecCoreDecoderType : u32 {
    VdecSpecial = 0,
    Vdec = 1,
    Vdec2 = 2,
    VdecSw = 3,
    Vdec2Special1 = 4,
    Vdec2Special2 = 8,
};

enum class OrbisVdecCoreCodecType : u32 {
    Avc = 0,
    Hevc = 4,
};

struct OrbisVdecCoreConfigInfo {
    OrbisVdecCoreDecoderType decoder_type;
    OrbisVdecCoreCodecType codec_type;
    s32 profile;
    s32 max_level;
    s32 max_dpb_frame_count;
    s32 max_frame_width;
    s32 max_frame_height;
    s32 horizontal_align;
    s32 surface_format; // 0 for normal, 6 for HDR
    s32 field_0x24;     // Always 0
    u8 hevc_tier_type;
    u8 frame_height_alignment;
    u8 max_bit_depth_luma;
    u8 max_bit_depth_chroma;
    s32 hevc_max_temporal_id_to_decode;
    u8 gpu_load_level;
    s32 field_0x34; // Never set
    s32 decode_pipeline_depth;
    s32 cpu_thread_priority;
    s32 no_optimize_progressive_video;
    s32 extra_dpb_frame_count;
    s64 cpu_affinity_mask;
    s32 max_pending_sync_count;
};
static_assert(sizeof(OrbisVdecCoreConfigInfo) == 0x58);

struct OrbisVdecCoreComputeResourceInfo {
    u64 memory_type;
    u64 size;
    u32 alignment;
    u32 reserved;
};
static_assert(sizeof(OrbisVdecCoreComputeResourceInfo) == 0x18);

struct OrbisVdecCoreMemoryInfo {
    u64 cpu_memory_size;
    void* p_cpu_memory;
    u64 cpu_gpu_memory_size;
    void* p_cpu_gpu_memory;
    u64 gpu_memory_size;
    void* p_gpu_memory;
};
static_assert(sizeof(OrbisVdecCoreMemoryInfo) == 0x30);

using SceVdecCoreComputeQueue = void*;
using SceVdecCoreDecoder = void*;

void RegisterLib(Core::Loader::SymbolsResolver* resolver);

} // namespace Libraries::VdecCore
