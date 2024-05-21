// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/bit_field.h"
#include "common/types.h"

#include <array>
#include <condition_variable>
#include <functional>
#include <future>
#include <span>
#include <thread>
#include <queue>

namespace Vulkan {
class Rasterizer;
}

namespace AmdGpu {

#define GFX6_3D_REG_INDEX(field_name) (offsetof(AmdGpu::Liverpool::Regs, field_name) / sizeof(u32))

#define CONCAT2(x, y) DO_CONCAT2(x, y)
#define DO_CONCAT2(x, y) x##y
#define INSERT_PADDING_WORDS(num_words)                                                            \
    [[maybe_unused]] std::array<u32, num_words> CONCAT2(pad, __LINE__)

struct Liverpool {
    static constexpr u32 NumColorBuffers = 8;
    static constexpr u32 NumViewports = 16;
    static constexpr u32 NumClipPlanes = 6;
    static constexpr u32 NumWordsShaderUserData = 16;
    static constexpr u32 UconfigRegWordOffset = 0xC000;
    static constexpr u32 ContextRegWordOffset = 0xA000;
    static constexpr u32 ShRegWordOffset = 0x2C00;
    static constexpr u32 NumRegs = 0xD000;

    using UserData = std::array<u32, NumWordsShaderUserData>;

    struct ShaderProgram {
        u32 address_lo;
        u32 address_hi;
        union {
            BitField<0, 6, u64> num_vgprs;
            BitField<6, 4, u64> num_sgprs;
            BitField<33, 5, u64> num_user_regs;
        } settings;
        UserData user_data;

        template <typename T = u8>
        const T* Address() const {
            const uintptr_t addr = uintptr_t(address_hi) << 40 | uintptr_t(address_lo) << 8;
            return reinterpret_cast<const T*>(addr);
        }
    };

    enum class ShaderExportComp : u32 {
        None = 0,
        OneComp = 1,
        TwoComp = 2,
        FourCompCompressed = 3,
        FourComp = 4,
    };

    union ShaderPosFormat {
        u32 raw;
        BitField<0, 4, ShaderExportComp> pos0;
        BitField<4, 4, ShaderExportComp> pos1;
        BitField<8, 4, ShaderExportComp> pos2;
        BitField<12, 4, ShaderExportComp> pos3;
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

    union ColorExportFormat {
        u32 raw;
        BitField<0, 4, ShaderExportFormat> col0;
        BitField<4, 4, ShaderExportFormat> col1;
        BitField<8, 4, ShaderExportFormat> col2;
        BitField<12, 4, ShaderExportFormat> col3;
        BitField<16, 4, ShaderExportFormat> col4;
        BitField<20, 4, ShaderExportFormat> col5;
        BitField<24, 4, ShaderExportFormat> col6;
        BitField<28, 4, ShaderExportFormat> col7;
    };

    union VsOutputControl {
        u32 raw;
        BitField<0, 8, u32> clip_distance_enable;
        BitField<8, 8, u32> cull_distance_enable;
        BitField<16, 1, u32> use_vtx_point_size;
        BitField<17, 1, u32> use_vtx_edge_flag;
        BitField<18, 1, u32> use_vtx_render_target_idx;
        BitField<19, 1, u32> use_vtx_viewport_idx;
        BitField<20, 1, u32> use_vtx_kill_flag;

        bool IsClipDistEnabled(u32 index) const {
            return (clip_distance_enable.Value() >> index) & 1;
        }

        bool IsCullDistEnabled(u32 index) const {
            return (cull_distance_enable.Value() >> index) & 1;
        }
    };

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

    union DepthBufferControl {
        u32 raw;
        BitField<0, 1, u32> z_export_enable;
        BitField<1, 1, u32> stencil_test_val_export_enable;
        BitField<2, 1, u32> stencil_op_val_export_enable;
        BitField<4, 2, ZOrder> z_order;
        BitField<6, 1, u32> kill_enable;
        BitField<7, 1, u32> coverage_to_mask_enable;
        BitField<8, 1, u32> mask_export_enable;
        BitField<9, 1, u32> exec_on_hier_fail;
        BitField<10, 1, u32> exec_on_noop;
        BitField<11, 1, u32> alpha_to_mask_disable;
        BitField<12, 1, u32> depth_before_shader;
        BitField<13, 2, ConservativeDepth> conservative_z_export;
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

    union DepthControl {
        u32 raw;
        BitField<0, 1, u32> stencil_enable;
        BitField<1, 1, u32> depth_enable;
        BitField<2, 1, u32> depth_write_enable;
        BitField<3, 1, u32> depth_bounds_enable;
        BitField<4, 3, CompareFunc> depth_func;
        BitField<7, 1, u32> backface_enable;
        BitField<8, 3, CompareFunc> stencil_ref_func;
        BitField<20, 3, CompareFunc> stencil_bf_func;
        BitField<30, 1, u32> enable_color_writes_on_depth_fail;
        BitField<31, 1, u32> disable_color_writes_on_depth_pass;
    };

    union DepthSize {
        u32 raw;
        BitField<0, 11, u32> pitch_tile_max;
        BitField<11, 11, u32> height_tile_max;

        u32 Pitch() const {
            return (pitch_tile_max + 1) << 3;
        }

        u32 Height() const {
            return (height_tile_max + 1) << 3;
        }
    };

    union DepthSlice {
        u32 raw;
        BitField<0, 22, u32> slice_tile_max;
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

    union StencilControl {
        u32 raw;
        BitField<0, 4, StencilFunc> stencil_fail_front;
        BitField<4, 4, StencilFunc> stencil_zpass_front;
        BitField<8, 4, StencilFunc> stencil_zfail_front;
        BitField<12, 4, StencilFunc> stencil_fail_back;
        BitField<16, 4, StencilFunc> stencil_zpass_back;
        BitField<20, 4, StencilFunc> stencil_zfail_back;
    };

    union StencilRefMask {
        u32 raw;
        BitField<0, 8, u32> stencil_test_val;
        BitField<8, 8, u32> stencil_mask;
        BitField<16, 8, u32> stencil_write_mask;
        BitField<24, 8, u32> stencil_op_val;
    };

    union StencilInfo {
        u32 raw;
        BitField<0, 1, u32> format;
    };

    enum class ClipSpace : u32 {
        MinusWToW = 0,
        ZeroToW = 1,
    };

    enum class PrimKillCond : u32 {
        AllVtx = 0,
        AnyVtx = 1,
    };

    union ClipperControl {
        u32 raw;
        BitField<0, 6, u32> user_clip_plane_enable;
        BitField<16, 1, u32> clip_disable;
        BitField<19, 1, ClipSpace> clip_space;
        BitField<21, 1, PrimKillCond> vtx_kill_or;
        BitField<22, 1, u32> dx_rasterization_kill;
        BitField<23, 1, u32> dx_linear_attr_clip_enable;
        BitField<26, 1, u32> zclip_near_disable;
        BitField<26, 1, u32> zclip_far_disable;
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

    union PolygonControl {
        u32 raw;
        BitField<0, 1, u32> cull_front;
        BitField<1, 1, u32> cull_back;
        BitField<3, 2, u32> enable_polygon_mode;
        BitField<5, 3, PolygonMode> polygon_mode_front;
        BitField<8, 3, PolygonMode> polygon_mode_back;
        BitField<11, 1, u32> enable_polygon_offset_front;
        BitField<12, 1, u32> enable_polygon_offset_back;
        BitField<13, 1, u32> enable_polygon_offset_para;
        BitField<13, 1, u32> enable_window_offset;
        BitField<19, 1, ProvokingVtxLast> provoking_vtx_last;

        PolygonMode PolyMode() const {
            return enable_polygon_mode ? polygon_mode_front.Value() : PolygonMode::Fill;
        }

        CullMode CullingMode() const {
            return static_cast<CullMode>(cull_front | cull_back << 1);
        }
    };

    union VsOutputConfig {
        u32 raw;
        BitField<1, 5, u32> export_count_min_one;
        BitField<6, 1, u32> half_pack;

        u32 NumExports() const {
            return export_count_min_one.Value() + 1;
        }
    };

    union ColorBufferMask {
        u32 raw;
        BitField<0, 4, u32> output0_mask;
        BitField<4, 4, u32> output1_mask;
        BitField<8, 4, u32> output2_mask;
        BitField<12, 4, u32> output3_mask;
        BitField<16, 4, u32> output4_mask;
        BitField<20, 4, u32> output5_mask;
        BitField<24, 4, u32> output6_mask;
        BitField<28, 4, u32> output7_mask;
    };

    struct IndexBufferBase {
        BitField<0, 8, u32> base_addr_hi;
        u32 base_addr_lo;

        VAddr Address() const {
            return base_addr_lo | u64(base_addr_hi) << 32;
        }
    };

    enum class IndexType : u32 {
        Index16 = 0,
        Index32 = 1,
    };

    enum class IndexSwapMode : u32 {
        None = 0,
        Swap16 = 1,
        Swap32 = 2,
        SwapWord = 3,
    };

    union IndexBufferType {
        u32 raw;
        BitField<0, 2, IndexType> index_type;
        BitField<2, 2, IndexSwapMode> swap_mode;
    };

    union VgtNumInstances {
        u32 num_instances;

        u32 NumInstances() const {
            return num_instances == 0 ? 1 : num_instances;
        }
    };

    struct Scissor {
        union {
            BitField<0, 16, s32> top_left_x;
            BitField<16, 16, s32> top_left_y;
        };
        union {
            BitField<0, 15, u32> bottom_right_x;
            BitField<16, 15, u32> bottom_right_y;
        };

        u32 GetWidth() const {
            return static_cast<u32>(bottom_right_x - top_left_x);
        }

        u32 GetHeight() const {
            return static_cast<u32>(bottom_right_y - top_left_y);
        }
    };

    struct ViewportScissor {
        union {
            BitField<0, 15, s32> top_left_x;
            BitField<15, 15, s32> top_left_y;
            BitField<30, 1, s32> window_offset_disble;
        };
        union {
            BitField<0, 15, s32> bottom_right_x;
            BitField<15, 15, s32> bottom_right_y;
        };
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
        float zoffset;
        float zscale;
    };

    union ViewportControl {
        BitField<0, 1, u32> xscale_enable;
        BitField<1, 1, u32> xoffset_enable;
        BitField<2, 1, u32> yscale_enable;
        BitField<3, 1, u32> yoffset_enable;
        BitField<4, 1, u32> zscale_enable;
        BitField<5, 1, u32> zoffset_enable;
        BitField<8, 1, u32> xy_transformed;
        BitField<9, 1, u32> z_transformed;
        BitField<10, 1, u32> w_transformed;
    };

    struct ClipUserData {
        u32 data_x;
        u32 data_y;
        u32 data_z;
        u32 data_w;
    };

    struct ColorBuffer {
        enum class EndianSwap : u32 {
            None = 0,
            Swap8In16 = 1,
            Swap8In32 = 2,
            Swap8In64 = 3,
        };

        enum class Format : u32 {
            Invalid = 0,
            Color_8 = 1,
            Color_16 = 2,
            Color_8_8 = 3,
            Color_32 = 4,
            Color_16_16 = 5,
            Color_10_11_11 = 6,
            Color_11_11_10 = 7,
            Color_10_10_10_2 = 8,
            Color_2_10_10_10 = 9,
            Color_8_8_8_8 = 10,
            Color_32_32 = 11,
            Color_16_16_16_16 = 12,
            Color_32_32_32_32 = 14,
            Color_5_6_5 = 16,
            Color_1_5_5_5 = 17,
            Color_5_5_5_1 = 18,
            Color_4_4_4_4 = 19,
            Color_8_24 = 20,
            Color_24_8 = 21,
            Color_X24_8_32_FL = 22,
        };

        enum class NumberType : u32 {
            Unorm = 0,
            Snorm = 1,
            Uint = 4,
            Sint = 5,
            Srgb = 6,
            Float = 7,
        };

        enum class SwapMode : u32 {
            Standard = 0,
            Alternate = 1,
            StandardReverse = 2,
            AlternateReverse = 3,
        };

        enum class RoundMode : u32 {
            ByHalf = 0,
            Truncate = 1,
        };

        u32 base_address;
        union {
            BitField<0, 11, u32> tile_max;
            BitField<20, 11, u32> fmask_tile_max;
        } pitch;
        union {
            BitField<0, 22, u32> tile_max;
        } slice;
        union {
            BitField<0, 11, u32> slice_start;
            BitField<13, 11, u32> slice_max;
        } view;
        union {
            BitField<0, 2, EndianSwap> endian;
            BitField<2, 5, Format> format;
            BitField<7, 1, u32> linear_general;
            BitField<8, 2, NumberType> number_type;
            BitField<11, 2, SwapMode> comp_swap;
            BitField<13, 1, u32> fast_clear;
            BitField<14, 1, u32> compression;
            BitField<15, 1, u32> blend_clamp;
            BitField<16, 1, u32> blend_bypass;
            BitField<17, 1, u32> simple_float;
            BitField<18, 1, RoundMode> round_mode;
            BitField<19, 1, u32> cmask_is_linear;
        } info;
        union {
            BitField<0, 5, u32> tile_mode_index;
            BitField<5, 5, u32> fmask_tile_mode_index;
            BitField<12, 3, u32> num_samples_log2;
            BitField<15, 3, u32> num_fragments_log2;
            BitField<18, 1, u32> force_dst_alpha_1;
        } attrib;
        INSERT_PADDING_WORDS(1);
        u32 cmask_base_address;
        union {
            BitField<0, 14, u32> tile_max;
        } cmask_slice;
        u32 fmask_base_address;
        union {
            BitField<0, 14, u32> tile_max;
        } fmask_slice;
        u32 clear_word0;
        u32 clear_word1;
        INSERT_PADDING_WORDS(2);

        u32 Pitch() const {
            return (pitch.tile_max + 1) << 3;
        }

        u32 Height() const {
            return (slice.tile_max + 1) * 64 / Pitch();
        }

        u64 Address() const {
            return u64(base_address) << 8;
        }

        u64 CmaskAddress() const {
            return u64(cmask_base_address) << 8;
        }
    };

    enum class PrimitiveType : u32 {
        None = 0,
        PointList = 1,
        LineList = 2,
        LineStrip = 3,
        TriangleList = 4,
        TriangleFan = 5,
        TriangleStrip = 6,
        PatchPrimitive = 9,
        AdjLineList = 10,
        AdjLineStrip = 11,
        AdjTriangleList = 12,
        AdjTriangleStrip = 13,
        RectList = 17,
        LineLoop = 18,
        QuadList = 19,
        QuadStrip = 20,
        Polygon = 21,
    };

    union Regs {
        struct {
            INSERT_PADDING_WORDS(0x2C08);
            ShaderProgram ps_program;
            INSERT_PADDING_WORDS(0x2C);
            ShaderProgram vs_program;
            INSERT_PADDING_WORDS(0xA008 - 0x2C4C - 16);
            u32 depth_bounds_min;
            u32 depth_bounds_max;
            u32 stencil_clear;
            u32 depth_clear;
            Scissor screen_scissor;
            INSERT_PADDING_WORDS(0xA011 - 0xA00C - 2);
            StencilInfo stencil_info;
            u32 z_read_base;
            u32 stencil_read_base;
            u32 z_write_base;
            u32 stencil_write_base;
            DepthSize depth_size;
            DepthSlice depth_slice;
            INSERT_PADDING_WORDS(0xA08E - 0xA018);
            ColorBufferMask color_target_mask;
            ColorBufferMask color_shader_mask;
            INSERT_PADDING_WORDS(0xA094 - 0xA08E - 2);
            std::array<ViewportScissor, NumViewports> viewport_scissors;
            std::array<ViewportDepth, NumViewports> viewport_depths;
            INSERT_PADDING_WORDS(0xA10B - 0xA0D4);
            StencilControl stencil_control;
            StencilRefMask stencil_ref_front;
            StencilRefMask stencil_ref_back;
            INSERT_PADDING_WORDS(1);
            std::array<ViewportBounds, NumViewports> viewports;
            std::array<ClipUserData, NumClipPlanes> clip_user_data;
            INSERT_PADDING_WORDS(0xA1B1 - 0xA187);
            VsOutputConfig vs_output_config;
            INSERT_PADDING_WORDS(0xA1C3 - 0xA1B1 - 1);
            ShaderPosFormat shader_pos_format;
            ShaderExportFormat z_export_format;
            ColorExportFormat color_export_format;
            INSERT_PADDING_WORDS(0xA1F9 - 0xA1C3 - 3);
            IndexBufferBase index_base_address;
            INSERT_PADDING_WORDS(1);
            u32 draw_initiator;
            INSERT_PADDING_WORDS(0xA200 - 0xA1F9 - 4);
            DepthControl depth_control;
            INSERT_PADDING_WORDS(2);
            DepthBufferControl depth_buffer_control;
            ClipperControl clipper_control;
            PolygonControl polygon_control;
            ViewportControl viewport_control;
            VsOutputControl vs_output_control;
            INSERT_PADDING_WORDS(0xA29E - 0xA207 - 1);
            u32 max_index_size;
            IndexBufferType index_buffer_type;
            INSERT_PADDING_WORDS(0xA2A1 - 0xA29E - 2);
            u32 enable_primitive_id;
            INSERT_PADDING_WORDS(0xA318 - 0xA2A1 - 1);
            ColorBuffer color_buffers[NumColorBuffers];
            INSERT_PADDING_WORDS(0xC242 - 0xA390);
            PrimitiveType primitive_type;
            INSERT_PADDING_WORDS(0xC24C - 0xC243);
            u32 num_indices;
            VgtNumInstances num_instances;
        };
        std::array<u32, NumRegs> reg_array{};
    };

    Regs regs{};

public:
    Liverpool();
    ~Liverpool();

    void SubmitGfx(std::span<const u32> dcb, std::span<const u32> ccb) {
        {
            std::scoped_lock lock{m_ring_access};
            gfx_ring.emplace(dcb);

            ASSERT_MSG(ccb.size() == 0, "CCBs are not supported yet");
        }
        cv_submit.notify_one();
    }

    void WaitGpuIdle();

    void BindRasterizer(Vulkan::Rasterizer* rasterizer_) {
        rasterizer = rasterizer_;
    }

private:
    void ProcessCmdList(const u32* cmdbuf, u32 size_in_bytes);
    void Process(std::stop_token stoken);

    Vulkan::Rasterizer* rasterizer;
    std::jthread process_thread{};
    std::queue<std::span<const u32>> gfx_ring{};
    std::condition_variable_any cv_submit{};
    std::condition_variable cv_complete{};
    std::mutex m_ring_access{};
};

static_assert(GFX6_3D_REG_INDEX(ps_program) == 0x2C08);
static_assert(GFX6_3D_REG_INDEX(vs_program) == 0x2C48);
static_assert(GFX6_3D_REG_INDEX(vs_program.user_data) == 0x2C4C);
static_assert(GFX6_3D_REG_INDEX(screen_scissor) == 0xA00C);
static_assert(GFX6_3D_REG_INDEX(depth_slice) == 0xA017);
static_assert(GFX6_3D_REG_INDEX(color_target_mask) == 0xA08E);
static_assert(GFX6_3D_REG_INDEX(color_shader_mask) == 0xA08F);
static_assert(GFX6_3D_REG_INDEX(viewport_scissors) == 0xA094);
static_assert(GFX6_3D_REG_INDEX(stencil_control) == 0xA10B);
static_assert(GFX6_3D_REG_INDEX(viewports) == 0xA10F);
static_assert(GFX6_3D_REG_INDEX(clip_user_data) == 0xA16F);
static_assert(GFX6_3D_REG_INDEX(vs_output_config) == 0xA1B1);
static_assert(GFX6_3D_REG_INDEX(shader_pos_format) == 0xA1C3);
static_assert(GFX6_3D_REG_INDEX(z_export_format) == 0xA1C4);
static_assert(GFX6_3D_REG_INDEX(color_export_format) == 0xA1C5);
static_assert(GFX6_3D_REG_INDEX(index_base_address) == 0xA1F9);
static_assert(GFX6_3D_REG_INDEX(draw_initiator) == 0xA1FC);
static_assert(GFX6_3D_REG_INDEX(clipper_control) == 0xA204);
static_assert(GFX6_3D_REG_INDEX(viewport_control) == 0xA206);
static_assert(GFX6_3D_REG_INDEX(vs_output_control) == 0xA207);
static_assert(GFX6_3D_REG_INDEX(index_buffer_type) == 0xA29F);
static_assert(GFX6_3D_REG_INDEX(enable_primitive_id) == 0xA2A1);
static_assert(GFX6_3D_REG_INDEX(color_buffers[0].base_address) == 0xA318);
static_assert(GFX6_3D_REG_INDEX(color_buffers[0].pitch) == 0xA319);
static_assert(GFX6_3D_REG_INDEX(color_buffers[0].slice) == 0xA31A);
static_assert(GFX6_3D_REG_INDEX(color_buffers[7].base_address) == 0xA381);
static_assert(GFX6_3D_REG_INDEX(primitive_type) == 0xC242);
static_assert(GFX6_3D_REG_INDEX(num_instances) == 0xC24D);

#undef GFX6_3D_REG_INDEX

} // namespace AmdGpu
