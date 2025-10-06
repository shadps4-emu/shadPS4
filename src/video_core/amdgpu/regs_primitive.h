// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace AmdGpu {

static constexpr u32 NUM_VIEWPORTS = 16;
static constexpr u32 NUM_CLIP_PLANES = 6;

enum class ClipSpace : u32 {
    MinusWToW = 0,
    ZeroToW = 1,
};

enum class PrimKillCond : u32 {
    AllVtx = 0,
    AnyVtx = 1,
};

struct ClipperControl {
    u32 user_clip_plane_enable : 6;
    u32 : 10;
    u32 clip_disable : 1;
    u32 : 2;
    ClipSpace clip_space : 1;
    u32 : 1;
    PrimKillCond vtx_kill_or : 1;
    u32 dx_rasterization_kill : 1;
    u32 : 1;
    u32 dx_linear_attr_clip_enable : 1;
    u32 : 1;
    u32 zclip_near_disable : 1;
    u32 zclip_far_disable : 1;

    bool ZclipEnable() const {
        if (zclip_near_disable != zclip_far_disable) {
            return false;
        }
        return !zclip_near_disable;
    }
};

enum class PolygonMode : u32 {
    Point = 0,
    Line = 1,
    Fill = 2,
};

enum class ProvokingVtxLast : u32 {
    First = 0,
    Last = 1,
};

enum class CullMode : u32 {
    None = 0,
    Front = 1,
    Back = 2,
    FrontAndBack = 3,
};

enum class FrontFace : u32 {
    CounterClockwise = 0,
    Clockwise = 1,
};

struct PolygonControl {
    u32 cull_front : 1;
    u32 cull_back : 1;
    FrontFace front_face : 1;
    u32 enable_polygon_mode : 2;
    PolygonMode polygon_mode_front : 3;
    PolygonMode polygon_mode_back : 3;
    u32 enable_polygon_offset_front : 1;
    u32 enable_polygon_offset_back : 1;
    u32 enable_polygon_offset_para : 1;
    u32 : 2;
    u32 enable_window_offset : 1;
    u32 : 2;
    ProvokingVtxLast provoking_vtx_last : 1;
    u32 persp_corr_dis : 1;
    u32 multi_prim_ib_ena : 1;

    PolygonMode PolyMode() const {
        return enable_polygon_mode ? polygon_mode_front : PolygonMode::Fill;
    }

    CullMode CullingMode() const {
        return static_cast<CullMode>(cull_front | cull_back << 1);
    }

    bool NeedsBias() const {
        return enable_polygon_offset_back || enable_polygon_offset_front ||
               enable_polygon_offset_para;
    }
};

struct VsOutputControl {
    u32 clip_distance_enable : 8;
    u32 cull_distance_enable : 8;
    u32 use_vtx_point_size : 1;
    u32 use_vtx_edge_flag : 1;
    u32 use_vtx_render_target_idx : 1;
    u32 use_vtx_viewport_idx : 1;
    u32 use_vtx_kill_flag : 1;
    u32 vs_out_misc_enable : 1;
    u32 vs_out_ccdist0_enable : 1;
    u32 vs_out_ccdist1_enable : 1;
    u32 vs_out_misc_side_bus_ena : 1;
    u32 use_vtx_gs_cut_flag : 1;

    bool IsClipDistEnabled(u32 index) const {
        return (clip_distance_enable >> index) & 1;
    }

    bool IsCullDistEnabled(u32 index) const {
        return (cull_distance_enable >> index) & 1;
    }
};

struct LineControl {
    u32 width_fixed_point;

    float Width() const {
        return static_cast<float>(width_fixed_point) / 8.0;
    }
};

struct ModeControl {
    u32 msaa_enable : 1;
    u32 vport_scissor_enable : 1;
    u32 line_stripple_enable : 1;
    u32 send_unlit_stiles_to_pkr : 1;
};

struct Scissor {
    struct {
        s16 top_left_x;
        s16 top_left_y;
    };
    struct {
        s16 bottom_right_x;
        s16 bottom_right_y;
    };

    static u16 Clamp(s16 value) {
        return std::max(s16(0), value);
    }

    u32 GetWidth() const {
        return static_cast<u32>(Clamp(bottom_right_x) - Clamp(top_left_x));
    }

    u32 GetHeight() const {
        return static_cast<u32>(Clamp(bottom_right_y) - Clamp(top_left_y));
    }
};

struct WindowOffset {
    s32 window_x_offset : 16;
    s32 window_y_offset : 16;
};

struct ViewportScissor {
    struct {
        u16 top_left_x : 15;
        u16 top_left_y : 15;
        u16 window_offset_disable : 1;
    };
    struct {
        u16 bottom_right_x : 15;
        u16 bottom_right_y : 15;
    };

    u32 GetWidth() const {
        return bottom_right_x - top_left_x;
    }

    u32 GetHeight() const {
        return bottom_right_y - top_left_y;
    }
};

struct ViewportDepth {
    float zmin;
    float zmax;
};

struct ViewportBounds {
    float xscale;
    float xoffset;
    float yscale;
    float yoffset;
    float zscale;
    float zoffset;
};

struct ViewportControl {
    u32 xscale_enable : 1;
    u32 xoffset_enable : 1;
    u32 yscale_enable : 1;
    u32 yoffset_enable : 1;
    u32 zscale_enable : 1;
    u32 zoffset_enable : 1;
    u32 : 2;
    u32 xy_transformed : 1;
    u32 z_transformed : 1;
    u32 w_transformed : 1;
    u32 perfcounter_ref : 1;
};

struct ClipUserData {
    u32 data_x;
    u32 data_y;
    u32 data_z;
    u32 data_w;
};

struct AaConfig {
    u32 msaa_num_samples : 3;
    u32 : 1;
    u32 aa_mask_centroid_dtmn : 1;
    u32 : 8;
    u32 max_sample_dst : 4;
    u32 : 3;
    u32 msaa_exposed_samples : 3;
    u32 : 1;
    u32 detail_to_exposed_mode : 2;

    u32 NumSamples() const {
        return 1 << msaa_num_samples;
    }
};

} // namespace AmdGpu
