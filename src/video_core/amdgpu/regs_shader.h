// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/types.h"
#include "shader_recompiler/params.h"

namespace AmdGpu {

static constexpr u32 NUM_USER_DATA = 16;

using UserData = std::array<u32, NUM_USER_DATA>;

struct BinaryInfo {
    static constexpr std::array<u8, 7> signature_ref = {0x4f, 0x72, 0x62, 0x53,
                                                        0x68, 0x64, 0x72}; // OrbShdr

    std::array<u8, sizeof(signature_ref)> signature;
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

    bool Valid() const {
        return signature == signature_ref;
    }
};

enum class FpRoundMode : u32 {
    NearestEven = 0,
    PlusInf = 1,
    MinInf = 2,
    ToZero = 3,
};

enum class FpDenormMode : u32 {
    InOutFlush = 0,
    InAllowOutFlush = 1,
    InFlushOutAllow = 2,
    InOutAllow = 3,
};

struct ShaderProgram {
    u64 address : 40;
    struct {
        u32 num_vgprs : 6;
        u32 num_sgprs : 4;
        u32 priority : 2;
        FpRoundMode fp_round_mode32 : 2;
        FpRoundMode fp_round_mode64 : 2;
        FpDenormMode fp_denorm_mode32 : 2;
        FpDenormMode fp_denorm_mode64 : 2;
        u32 : 4;
        u32 vgpr_comp_cnt : 2;
        u32 : 6;
        u32 scratch_en : 1;
        u32 num_user_regs : 5;
        u32 : 1;
        u32 oc_lds_en : 1;
    } settings;
    UserData user_data;

    template <typename T = u8*>
    const T Address() const {
        return std::bit_cast<T>(address << 8);
    }

    [[nodiscard]] u32 NumVgprs() const {
        // Each increment allocates 4 registers, where 0 = 4 registers.
        return (settings.num_vgprs + 1) * 4;
    }
};

struct VsOutputConfig {
    u32 : 1;
    u32 export_count_min_one : 5;
    u32 half_pack : 1;

    u32 NumExports() const {
        return export_count_min_one + 1;
    }
};

struct PsInputControl {
    u32 input_offset : 5;
    u32 use_default : 1;
    u32 : 2;
    u32 default_value : 2;
    u32 flat_shade : 1;
};

struct PsInput {
    u32 persp_sample_ena : 1;
    u32 persp_center_ena : 1;
    u32 persp_centroid_ena : 1;
    u32 persp_pull_model_ena : 1;
    u32 linear_sample_ena : 1;
    u32 linear_center_ena : 1;
    u32 linear_centroid_ena : 1;
    u32 line_stipple_tex_ena : 1;
    u32 pos_x_float_ena : 1;
    u32 pos_y_float_ena : 1;
    u32 pos_z_float_ena : 1;
    u32 pos_w_float_ena : 1;
    u32 front_face_ena : 1;
    u32 ancillary_ena : 1;
    u32 sample_coverage_ena : 1;
    u32 pos_fixed_pt_ena : 1;

    bool operator==(const PsInput&) const = default;
};

enum class ShaderExportComp : u32 {
    None = 0,
    OneComp = 1,
    TwoComp = 2,
    FourCompCompressed = 3,
    FourComp = 4,
};

struct ShaderPosFormat {
    ShaderExportComp pos0 : 4;
    ShaderExportComp pos1 : 4;
    ShaderExportComp pos2 : 4;
    ShaderExportComp pos3 : 4;
};

enum class ShaderExportFormat : u32 {
    Zero = 0,
    R_32 = 1,
    GR_32 = 2,
    AR_32 = 3,
    ABGR_FP16 = 4,
    ABGR_UNORM16 = 5,
    ABGR_SNORM16 = 6,
    ABGR_UINT16 = 7,
    ABGR_SINT16 = 8,
    ABGR_32 = 9,
};

struct ColorExportFormat {
    u32 raw;

    [[nodiscard]] ShaderExportFormat GetFormat(const u32 buf_idx) const {
        return static_cast<ShaderExportFormat>((raw >> (buf_idx * 4)) & 0xfu);
    }
};

struct ComputeProgram {
    u32 dispatch_initiator;
    u32 dim_x;
    u32 dim_y;
    u32 dim_z;
    u32 start_x;
    u32 start_y;
    u32 start_z;
    struct {
        u16 full;
        u16 partial;
    } num_thread_x, num_thread_y, num_thread_z;
    u32 pad0;
    u32 max_wave_id : 12;
    u64 address : 40;
    std::array<u32, 4> pad1;
    struct {
        u64 num_vgprs : 6;
        u64 num_sgprs : 4;
        u64 : 23;
        u64 num_user_regs : 5;
        u64 : 1;
        u64 tgid_enable : 3;
        u64 : 5;
        u64 lds_dwords : 9;
    } settings;
    u32 pad2;
    u32 resource_limits;
    std::array<u32, 42> pad3;
    UserData user_data;

    template <typename T = u8*>
    const T Address() const {
        return std::bit_cast<T>(address << 8);
    }

    u32 SharedMemSize() const noexcept {
        // lds_dwords is in units of 128 dwords. We return bytes.
        return settings.lds_dwords * 128 * 4;
    }

    u32 NumWorkgroups() const noexcept {
        return dim_x * dim_y * dim_z;
    }

    bool IsTgidEnabled(u32 i) const noexcept {
        return (settings.tgid_enable >> i) & 1;
    }
};

static constexpr const BinaryInfo& SearchBinaryInfo(const u32* code) {
    constexpr u32 token_mov_vcchi = 0xBEEB03FF;
    if (code[0] == token_mov_vcchi) {
        const auto* info = std::bit_cast<const BinaryInfo*>(code + (code[1] + 1) * 2);
        if (info->Valid()) {
            return *info;
        }
    }
    constexpr u32 signature_size = sizeof(BinaryInfo::signature_ref) / sizeof(u8);
    constexpr u32 search_limit = 0x4000;
    const u32* end = code + search_limit;
    for (const u32* it = code; it < end; ++it) {
        if (const BinaryInfo* info = std::bit_cast<const BinaryInfo*>(it); info->Valid()) {
            return *info;
        }
    }
    UNREACHABLE_MSG("Shader binary info not found.");
}

static constexpr Shader::ShaderParams GetParams(const auto& sh) {
    const auto* code = sh.template Address<u32*>();
    const auto& bininfo = SearchBinaryInfo(code);
    return {
        .user_data = sh.user_data,
        .code = std::span{code, bininfo.length / sizeof(u32)},
        .hash = bininfo.shader_hash,
    };
}

} // namespace AmdGpu
