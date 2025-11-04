// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/types.h"
#include "video_core/amdgpu/tiling.h"

namespace AmdGpu {

enum class ZOrder : u32 {
    LateZ = 0,
    EarlyZLateZ = 1,
    ReZ = 2,
    EarlyZReZ = 3,
};

enum class ConservativeDepth : u32 {
    Any = 0,
    LessThanZ = 1,
    GreaterThanZ = 2,
};

struct DepthShaderControl {
    u32 z_export_enable : 1;
    u32 stencil_test_val_export_enable : 1;
    u32 stencil_op_val_export_enable : 1;
    u32 : 1;
    ZOrder z_order : 2;
    u32 kill_enable : 1;
    u32 coverage_to_mask_enable : 1;
    u32 mask_export_enable : 1;
    u32 exec_on_hier_fail : 1;
    u32 exec_on_noop : 1;
    u32 alpha_to_mask_disable : 1;
    u32 depth_before_shader : 1;
    ConservativeDepth conservative_z_export : 2;
};

enum class CompareFunc : u32 {
    Never = 0,
    Less = 1,
    Equal = 2,
    LessEqual = 3,
    Greater = 4,
    NotEqual = 5,
    GreaterEqual = 6,
    Always = 7,
};

struct DepthControl {
    u32 stencil_enable : 1;
    u32 depth_enable : 1;
    u32 depth_write_enable : 1;
    u32 depth_bounds_enable : 1;
    CompareFunc depth_func : 3;
    u32 backface_enable : 1;
    CompareFunc stencil_ref_func : 3;
    u32 : 9;
    CompareFunc stencil_bf_func : 3;
    u32 : 7;
    u32 enable_color_writes_on_depth_fail : 1;
    u32 disable_color_writes_on_depth_pass : 1;
};

enum class StencilFunc : u32 {
    Keep = 0,
    Zero = 1,
    Ones = 2,
    ReplaceTest = 3,
    ReplaceOp = 4,
    AddClamp = 5,
    SubClamp = 6,
    Invert = 7,
    AddWrap = 8,
    SubWrap = 9,
    And = 10,
    Or = 11,
    Xor = 12,
    Nand = 13,
    Nor = 14,
    Xnor = 15,
};

struct StencilControl {
    StencilFunc stencil_fail_front : 4;
    StencilFunc stencil_zpass_front : 4;
    StencilFunc stencil_zfail_front : 4;
    StencilFunc stencil_fail_back : 4;
    StencilFunc stencil_zpass_back : 4;
    StencilFunc stencil_zfail_back : 4;
};

struct StencilRefMask {
    u8 stencil_test_val;
    u8 stencil_mask;
    u8 stencil_write_mask;
    u8 stencil_op_val;
};

struct DepthRenderControl {
    u32 depth_clear_enable : 1;
    u32 stencil_clear_enable : 1;
    u32 depth_copy : 1;
    u32 stencil_copy : 1;
    u32 resummarize_enable : 1;
    u32 stencil_compress_disable : 1;
    u32 depth_compress_disable : 1;
    u32 copy_centroid : 1;
    u32 copy_sample : 1;
    u32 decompress_enable : 1;
};

struct DepthView {
    u32 slice_start : 11;
    u32 : 2;
    u32 slice_max : 11;
    u32 z_read_only : 1;
    u32 stencil_read_only : 1;

    u32 NumSlices() const {
        return slice_max + 1u;
    }
};

enum class ForceEnable : u32 {
    Off = 0,
    Enable = 1,
    Disable = 2,
};

enum class ForceSumm : u32 {
    Off = 0,
    MinZ = 1,
    MaxZ = 2,
    Both = 3,
};

struct DepthRenderOverride {
    ForceEnable force_hiz_enable : 2;
    ForceEnable force_his_enable0 : 2;
    ForceEnable force_his_enable1 : 2;
    u32 force_shader_z_order : 1;
    u32 fast_z_disable : 1;
    u32 fast_stencil_disable : 1;
    u32 noop_cull_disable : 1;
    u32 force_color_kill : 1;
    u32 force_z_read : 1;
    u32 force_stencil_read : 1;
    ForceEnable force_full_z_range : 2;
    u32 force_qc_smask_conflict : 1;
    u32 disable_viewport_clamp : 1;
    u32 ignore_sc_zrange : 1;
    u32 disable_fully_covered : 1;
    ForceSumm force_z_limit_summ : 2;
    u32 max_tiles_in_dtt : 5;
    u32 disable_tile_rate_tiles : 1;
    u32 force_z_dirty : 1;
    u32 force_stencil_dirty : 1;
    u32 force_z_valid : 1;
    u32 force_stencil_valid : 1;
    u32 preserve_compression : 1;
};

struct Eqaa {
    u32 max_anchor_samples : 1;
    u32 : 3;
    u32 ps_iter_samples : 3;
    u32 : 1;
    u32 mask_export_num_samples : 3;
    u32 : 1;
    u32 alpha_to_mask_num_samples : 3;
    u32 : 1;
    u32 high_quality_intersections : 1;
    u32 incoherent_eqaa_reads : 1;
    u32 interpolate_comp_z : 1;
    u32 interpolate_src_z : 1;
    u32 static_anchor_associations : 1;
    u32 alpha_to_mask_eqaa_disable : 1;
    u32 : 2;
    u32 overrasterization_amount : 3;
    u32 enable_postz_overrasterization : 1;
};

struct DepthBuffer {
    enum class ZFormat : u32 {
        Invalid = 0,
        Z16 = 1,
        Z32Float = 3,
    };

    enum class StencilFormat : u32 {
        Invalid = 0,
        Stencil8 = 1,
    };

    struct ZInfo {
        ZFormat format : 2;
        u32 num_samples : 2;
        u32 : 9;
        u32 tile_split : 3;
        u32 : 4;
        u32 tile_mode_index : 3;
        u32 decompress_on_n_zplanes : 4;
        u32 allow_expclear : 1;
        u32 read_size : 1;
        u32 tile_surface_enable : 1;
        u32 clear_disallowed : 1;
        u32 zrange_precision : 1;
    } z_info;
    struct {
        StencilFormat format : 1;
    } stencil_info;
    u32 z_read_base;
    u32 stencil_read_base;
    u32 z_write_base;
    u32 stencil_write_base;
    struct {
        u32 pitch_tile_max : 11;
        u32 height_tile_max : 11;
    } depth_size;
    struct {
        u32 tile_max : 22;
    } depth_slice;

    bool DepthValid() const {
        return DepthAddress() != 0 && z_info.format != ZFormat::Invalid;
    }

    bool StencilValid() const {
        return StencilAddress() != 0 && stencil_info.format != StencilFormat::Invalid;
    }

    bool DepthWriteValid() const {
        return DepthWriteAddress() != 0 && z_info.format != ZFormat::Invalid;
    }

    bool StencilWriteValid() const {
        return StencilWriteAddress() != 0 && stencil_info.format != StencilFormat::Invalid;
    }

    u32 Pitch() const {
        return (depth_size.pitch_tile_max + 1) << 3;
    }

    u32 Height() const {
        return (depth_size.height_tile_max + 1) << 3;
    }

    u64 DepthAddress() const {
        return u64(z_read_base) << 8;
    }

    u64 StencilAddress() const {
        return u64(stencil_read_base) << 8;
    }

    u64 DepthWriteAddress() const {
        return u64(z_write_base) << 8;
    }

    u64 StencilWriteAddress() const {
        return u64(stencil_write_base) << 8;
    }

    u32 NumSamples() const {
        return 1u << z_info.num_samples; // spec doesn't say it is a log2
    }

    u32 NumBits() const {
        return z_info.format == ZFormat::Z32Float ? 32 : 16;
    }

    u32 GetDepthSliceSize() const {
        ASSERT(z_info.format != ZFormat::Invalid);
        const auto bpe = NumBits() >> 3; // in bytes
        return (depth_slice.tile_max + 1) * 64 * bpe * NumSamples();
    }

    TileMode GetTileMode() const {
        return static_cast<TileMode>(z_info.tile_mode_index);
    }

    bool IsTiled() const {
        return GetTileMode() != TileMode::DisplayLinearAligned &&
               GetTileMode() != TileMode::DisplayLinearGeneral;
    }
};

} // namespace AmdGpu
