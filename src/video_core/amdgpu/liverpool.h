// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <condition_variable>
#include <coroutine>
#include <exception>
#include <mutex>
#include <span>
#include <thread>
#include <vector>
#include <queue>

#include "common/assert.h"
#include "common/bit_field.h"
#include "common/polyfill_thread.h"
#include "common/types.h"
#include "common/unique_function.h"
#include "shader_recompiler/params.h"
#include "video_core/amdgpu/pixel_format.h"
#include "video_core/amdgpu/resource.h"

namespace Vulkan {
class Rasterizer;
}

namespace Libraries::VideoOut {
struct VideoOutPort;
}

namespace AmdGpu {

#define GFX6_3D_REG_INDEX(field_name) (offsetof(AmdGpu::Liverpool::Regs, field_name) / sizeof(u32))

#define CONCAT2(x, y) DO_CONCAT2(x, y)
#define DO_CONCAT2(x, y) x##y
#define INSERT_PADDING_WORDS(num_words)                                                            \
    [[maybe_unused]] std::array<u32, num_words> CONCAT2(pad, __LINE__)

struct Liverpool {
    static constexpr u32 GfxQueueId = 0u;
    static constexpr u32 NumGfxRings = 1u;     // actually 2, but HP is reserved by system software
    static constexpr u32 NumComputePipes = 7u; // actually 8, but #7 is reserved by system software
    static constexpr u32 NumQueuesPerPipe = 8u;
    static constexpr u32 NumTotalQueues = NumGfxRings + (NumComputePipes * NumQueuesPerPipe);
    static_assert(NumTotalQueues < 64u); // need to fit into u64 bitmap for ffs

    static constexpr u32 NumColorBuffers = 8;
    static constexpr u32 NumViewports = 16;
    static constexpr u32 NumClipPlanes = 6;
    static constexpr u32 NumShaderUserData = 16;
    static constexpr u32 UconfigRegWordOffset = 0xC000;
    static constexpr u32 ContextRegWordOffset = 0xA000;
    static constexpr u32 ConfigRegWordOffset = 0x2000;
    static constexpr u32 ShRegWordOffset = 0x2C00;
    static constexpr u32 NumRegs = 0xD000;

    using UserData = std::array<u32, NumShaderUserData>;

    struct BinaryInfo {
        static constexpr u8 signature_ref[] = {0x4f, 0x72, 0x62, 0x53, 0x68, 0x64, 0x72}; // OrbShdr

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
            return shader_hash && crc32 &&
                   (std::memcmp(signature.data(), signature_ref, sizeof(signature_ref)) == 0);
        }
    };

    struct ShaderProgram {
        u32 address_lo;
        BitField<0, 8, u32> address_hi;
        union {
            BitField<0, 6, u64> num_vgprs;
            BitField<6, 4, u64> num_sgprs;
            BitField<24, 2, u64> vgpr_comp_cnt; // SPI provided per-thread inputs
            BitField<33, 5, u64> num_user_regs;
        } settings;
        UserData user_data;

        template <typename T = u8*>
        const T Address() const {
            const uintptr_t addr = uintptr_t(address_hi) << 40 | uintptr_t(address_lo) << 8;
            return reinterpret_cast<const T>(addr);
        }

        std::span<const u32> Code() const {
            const u32* code = Address<u32*>();
            BinaryInfo bininfo;
            std::memcpy(&bininfo, code + (code[1] + 1) * 2, sizeof(bininfo));
            const u32 num_dwords = bininfo.length / sizeof(u32);
            return std::span{code, num_dwords};
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
        INSERT_PADDING_WORDS(1);
        BitField<0, 12, u32> max_wave_id;
        u32 address_lo;
        BitField<0, 8, u32> address_hi;
        INSERT_PADDING_WORDS(4);
        union {
            BitField<0, 6, u64> num_vgprs;
            BitField<6, 4, u64> num_sgprs;
            BitField<33, 5, u64> num_user_regs;
            BitField<39, 3, u64> tgid_enable;
            BitField<47, 9, u64> lds_dwords;
        } settings;
        INSERT_PADDING_WORDS(1);
        u32 resource_limits;
        INSERT_PADDING_WORDS(0x2A);
        UserData user_data;

        template <typename T = u8*>
        const T Address() const {
            const uintptr_t addr = uintptr_t(address_hi) << 40 | uintptr_t(address_lo) << 8;
            return reinterpret_cast<const T>(addr);
        }

        u32 SharedMemSize() const noexcept {
            // lds_dwords is in units of 128 dwords. We return bytes.
            return settings.lds_dwords.Value() * 128 * 4;
        }

        bool IsTgidEnabled(u32 i) const noexcept {
            return (settings.tgid_enable.Value() >> i) & 1;
        }

        std::span<const u32> Code() const {
            const u32* code = Address<u32*>();
            BinaryInfo bininfo;
            std::memcpy(&bininfo, code + (code[1] + 1) * 2, sizeof(bininfo));
            const u32 num_dwords = bininfo.length / sizeof(u32);
            return std::span{code, num_dwords};
        }
    };

    template <typename Shader>
    static constexpr auto* GetBinaryInfo(const Shader& sh) {
        const auto* code = sh.template Address<u32*>();
        const auto* bininfo = std::bit_cast<const BinaryInfo*>(code + (code[1] + 1) * 2);
        // ASSERT_MSG(bininfo->Valid(), "Invalid shader binary header");
        return bininfo;
    }

    static constexpr Shader::ShaderParams GetParams(const auto& sh) {
        auto* bininfo = GetBinaryInfo(sh);
        return {
            .user_data = sh.user_data,
            .code = sh.Code(),
            .hash = bininfo->shader_hash,
        };
    }

    union PsInputControl {
        u32 raw;
        BitField<0, 5, u32> input_offset;
        BitField<5, 1, u32> use_default;
        BitField<8, 2, u32> default_value;
        BitField<10, 1, u32> flat_shade;
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
        BitField<21, 1, u32> vs_out_misc_enable;
        BitField<22, 1, u32> vs_out_ccdist0_enable;
        BitField<23, 1, u32> vs_out_ccdist1_enable;
        BitField<25, 1, u32> use_vtx_gs_cut_flag;

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

        union {
            BitField<0, 2, ZFormat> format;
            BitField<2, 2, u32> num_samples;
            BitField<13, 3, u32> tile_split;
            BitField<27, 1, u32> allow_expclear;
            BitField<28, 1, u32> read_size;
            BitField<29, 1, u32> tile_surface_en;
            BitField<31, 1, u32> zrange_precision;
        } z_info;
        union {
            BitField<0, 1, StencilFormat> format;
        } stencil_info;
        u32 z_read_base;
        u32 stencil_read_base;
        u32 z_write_base;
        u32 stencil_write_base;
        union {
            BitField<0, 11, u32> pitch_tile_max;
            BitField<11, 11, u32> height_tile_max;
        } depth_size;
        union {
            BitField<0, 22, u32> tile_max;
        } depth_slice;

        u32 Pitch() const {
            return (depth_size.pitch_tile_max + 1) << 3;
        }

        u32 Height() const {
            return (depth_size.height_tile_max + 1) << 3;
        }

        u64 Address() const {
            return u64(z_read_base) << 8;
        }

        u32 NumSamples() const {
            return 1u << z_info.num_samples; // spec doesn't say it is a log2
        }

        u32 NumBits() const {
            return z_info.format == ZFormat::Z32Float ? 32 : 16;
        }

        size_t GetDepthSliceSize() const {
            ASSERT(z_info.format != ZFormat::Invalid);
            const auto bpe = NumBits() >> 3; // in bytes
            return (depth_slice.tile_max + 1) * 64 * bpe * NumSamples();
        }
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

    enum class FrontFace : u32 {
        CounterClockwise = 0,
        Clockwise = 1,
    };

    union PolygonControl {
        u32 raw;
        BitField<0, 1, u32> cull_front;
        BitField<1, 1, u32> cull_back;
        BitField<2, 1, FrontFace> front_face;
        BitField<3, 2, u32> enable_polygon_mode;
        BitField<5, 3, PolygonMode> polygon_mode_front;
        BitField<8, 3, PolygonMode> polygon_mode_back;
        BitField<11, 1, u32> enable_polygon_offset_front;
        BitField<12, 1, u32> enable_polygon_offset_back;
        BitField<13, 1, u32> enable_polygon_offset_para;
        BitField<16, 1, u32> enable_window_offset;
        BitField<19, 1, ProvokingVtxLast> provoking_vtx_last;

        PolygonMode PolyMode() const {
            return enable_polygon_mode ? polygon_mode_front.Value() : PolygonMode::Fill;
        }

        CullMode CullingMode() const {
            return static_cast<CullMode>(cull_front | cull_back << 1);
        }

        bool NeedsBias() const {
            return enable_polygon_offset_back || enable_polygon_offset_front ||
                   enable_polygon_offset_para;
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
        enum ColorComponent : u32 {
            ComponentR = (1u << 0),
            ComponentG = (1u << 1),
            ComponentB = (1u << 2),
            ComponentA = (1u << 3),
        };

        u32 raw;
        BitField<0, 4, u32> output0_mask;
        BitField<4, 4, u32> output1_mask;
        BitField<8, 4, u32> output2_mask;
        BitField<12, 4, u32> output3_mask;
        BitField<16, 4, u32> output4_mask;
        BitField<20, 4, u32> output5_mask;
        BitField<24, 4, u32> output6_mask;
        BitField<28, 4, u32> output7_mask;

        u32 GetMask(int buf_id) const {
            return (raw >> (buf_id * 4)) & 0xfu;
        }

        void SetMask(int buf_id, u32 mask) {
            raw &= ~(0xf << (buf_id * 4));
            raw |= (mask << (buf_id * 4));
        }
    };

    struct IndexBufferBase {
        BitField<0, 8, u32> base_addr_hi;
        u32 base_addr_lo;

        template <typename T = VAddr>
        T Address() const {
            return std::bit_cast<T>((base_addr_lo & ~1U) | u64(base_addr_hi) << 32);
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
        struct {
            s16 top_left_x;
            s16 top_left_y;
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
            BitField<30, 1, s32> window_offset_disable;
        };
        union {
            BitField<0, 15, s32> bottom_right_x;
            BitField<15, 15, s32> bottom_right_y;
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

    struct BlendConstants {
        float red;
        float green;
        float blue;
        float alpha;
    };

    union BlendControl {
        enum class BlendFactor : u32 {
            Zero = 0,
            One = 1,
            SrcColor = 2,
            OneMinusSrcColor = 3,
            SrcAlpha = 4,
            OneMinusSrcAlpha = 5,
            DstAlpha = 6,
            OneMinusDstAlpha = 7,
            DstColor = 8,
            OneMinusDstColor = 9,
            SrcAlphaSaturate = 10,
            ConstantColor = 13,
            OneMinusConstantColor = 14,
            Src1Color = 15,
            InvSrc1Color = 16,
            Src1Alpha = 17,
            InvSrc1Alpha = 18,
            ConstantAlpha = 19,
            OneMinusConstantAlpha = 20,
        };

        enum class BlendFunc : u32 {
            Add = 0,
            Subtract = 1,
            Min = 2,
            Max = 3,
            ReverseSubtract = 4,
        };

        BitField<0, 5, BlendFactor> color_src_factor;
        BitField<5, 3, BlendFunc> color_func;
        BitField<8, 5, BlendFactor> color_dst_factor;
        BitField<16, 5, BlendFactor> alpha_src_factor;
        BitField<21, 3, BlendFunc> alpha_func;
        BitField<24, 5, BlendFactor> alpha_dst_factor;
        BitField<29, 1, u32> separate_alpha_blend;
        BitField<30, 1, u32> enable;
    };

    union ColorControl {
        enum class OperationMode : u32 {
            Disable = 0u,
            Normal = 1u,
            EliminateFastClear = 2u,
            Resolve = 3u,
            FmaskDecompress = 5u,
        };

        BitField<3, 1, u32> degamma_enable;
        BitField<4, 3, OperationMode> mode;
        BitField<16, 8, u32> rop3;
    };

    struct ColorBuffer {
        enum class EndianSwap : u32 {
            None = 0,
            Swap8In16 = 1,
            Swap8In32 = 2,
            Swap8In64 = 3,
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
            BitField<2, 5, DataFormat> format;
            BitField<7, 1, u32> linear_general;
            BitField<8, 3, NumberFormat> number_type;
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
            BitField<0, 5, TilingMode> tile_mode_index;
            BitField<5, 5, u32> fmask_tile_mode_index;
            BitField<12, 3, u32> num_samples_log2;
            BitField<15, 2, u32> num_fragments_log2;
            BitField<17, 1, u32> force_dst_alpha_1;
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

        operator bool() const {
            return info.format != DataFormat::FormatInvalid;
        }

        u32 Pitch() const {
            return (pitch.tile_max + 1) << 3;
        }

        u32 Height() const {
            return (slice.tile_max + 1) * 64 / Pitch();
        }

        u64 Address() const {
            return u64(base_address) << 8;
        }

        VAddr CmaskAddress() const {
            return VAddr(cmask_base_address) << 8;
        }

        VAddr FmaskAddress() const {
            return VAddr(fmask_base_address) << 8;
        }

        u32 NumSamples() const {
            return 1 << attrib.num_fragments_log2;
        }

        u32 NumSlices() const {
            return view.slice_max + 1;
        }

        size_t GetColorSliceSize() const {
            const auto num_bytes_per_element = NumBits(info.format) / 8u;
            const auto slice_size =
                num_bytes_per_element * (slice.tile_max + 1) * 64u * NumSamples();
            return slice_size;
        }

        TilingMode GetTilingMode() const {
            return info.linear_general ? TilingMode::Display_Linear
                                       : attrib.tile_mode_index.Value();
        }

        bool IsTiled() const {
            return !info.linear_general;
        }

        NumberFormat NumFormat() const {
            // There is a small difference between T# and CB number types, account for it.
            return info.number_type == AmdGpu::NumberFormat::SnormNz ? AmdGpu::NumberFormat::Srgb
                                                                     : info.number_type.Value();
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

    enum ContextRegs : u32 {
        DbZInfo = 0xA010,
        CbColor0Base = 0xA318,
        CbColor1Base = 0xA327,
        CbColor2Base = 0xA336,
        CbColor3Base = 0xA345,
        CbColor4Base = 0xA354,
        CbColor5Base = 0xA363,
        CbColor6Base = 0xA372,
        CbColor7Base = 0xA381,
        CbColor0Cmask = 0xA31F,
        CbColor1Cmask = 0xA32E,
        CbColor2Cmask = 0xA33D,
        CbColor3Cmask = 0xA34C,
        CbColor4Cmask = 0xA35B,
        CbColor5Cmask = 0xA36A,
        CbColor6Cmask = 0xA379,
        CbColor7Cmask = 0xA388,
    };

    struct PolygonOffset {
        float depth_bias;
        float front_scale;
        float front_offset;
        float back_scale;
        float back_offset;
    };

    struct Address {
        u32 address;

        VAddr GetAddress() const {
            return u64(address) << 8;
        }
    };

    union DepthRenderControl {
        u32 raw;
        BitField<0, 1, u32> depth_clear_enable;
        BitField<1, 1, u32> stencil_clear_enable;
        BitField<5, 1, u32> stencil_compress_disable;
        BitField<6, 1, u32> depth_compress_disable;
    };

    union DepthView {
        BitField<0, 11, u32> slice_start;
        BitField<13, 11, u32> slice_max;
        BitField<24, 1, u32> z_read_only;
        BitField<25, 1, u32> stencil_read_only;

        u32 NumSlices() const {
            return slice_max + 1u;
        }
    };

    union AaConfig {
        BitField<0, 3, u32> msaa_num_samples;
        BitField<4, 1, u32> aa_mask_centroid_dtmn;
        BitField<13, 4, u32> max_sample_dst;
        BitField<20, 3, u32> msaa_exposed_samples;
        BitField<24, 2, u32> detail_to_exposed_mode;

        u32 NumSamples() const {
            return 1 << msaa_num_samples;
        }
    };

    union ShaderStageEnable {
        u32 raw;
        BitField<0, 2, u32> ls_en;
        BitField<2, 1, u32> hs_en;
        BitField<3, 2, u32> es_en;
        BitField<5, 1, u32> gs_en;
        BitField<6, 1, u32> vs_en;

        bool IsStageEnabled(u32 stage) {
            switch (stage) {
            case 0:
            case 1:
                return true;
            case 2:
                return gs_en.Value();
            case 3:
                return es_en.Value();
            case 4:
                return hs_en.Value();
            case 5:
                return ls_en.Value();
            default:
                UNREACHABLE();
            }
        }
    };

    union Regs {
        struct {
            INSERT_PADDING_WORDS(0x2C08);
            ShaderProgram ps_program;
            INSERT_PADDING_WORDS(0x2C);
            ShaderProgram vs_program;
            INSERT_PADDING_WORDS(0x2C);
            ShaderProgram gs_program;
            INSERT_PADDING_WORDS(0x2C);
            ShaderProgram es_program;
            INSERT_PADDING_WORDS(0x2C);
            ShaderProgram hs_program;
            INSERT_PADDING_WORDS(0x2C);
            ShaderProgram ls_program;
            INSERT_PADDING_WORDS(0xA4);
            ComputeProgram cs_program;
            INSERT_PADDING_WORDS(0xA008 - 0x2E00 - 80 - 3 - 5);
            DepthRenderControl depth_render_control;
            INSERT_PADDING_WORDS(1);
            DepthView depth_view;
            INSERT_PADDING_WORDS(2);
            Address depth_htile_data_base;
            INSERT_PADDING_WORDS(2);
            float depth_bounds_min;
            float depth_bounds_max;
            u32 stencil_clear;
            float depth_clear;
            Scissor screen_scissor;
            INSERT_PADDING_WORDS(0xA010 - 0xA00C - 2);
            DepthBuffer depth_buffer;
            INSERT_PADDING_WORDS(0xA08E - 0xA018);
            ColorBufferMask color_target_mask;
            ColorBufferMask color_shader_mask;
            INSERT_PADDING_WORDS(0xA094 - 0xA08E - 2);
            std::array<ViewportScissor, NumViewports> viewport_scissors;
            std::array<ViewportDepth, NumViewports> viewport_depths;
            INSERT_PADDING_WORDS(0xA103 - 0xA0D4);
            u32 primitive_restart_index;
            INSERT_PADDING_WORDS(1);
            BlendConstants blend_constants;
            INSERT_PADDING_WORDS(0xA10B - 0xA105 - 4);
            StencilControl stencil_control;
            StencilRefMask stencil_ref_front;
            StencilRefMask stencil_ref_back;
            INSERT_PADDING_WORDS(1);
            std::array<ViewportBounds, NumViewports> viewports;
            std::array<ClipUserData, NumClipPlanes> clip_user_data;
            INSERT_PADDING_WORDS(0xA191 - 0xA187);
            std::array<PsInputControl, 32> ps_inputs;
            VsOutputConfig vs_output_config;
            INSERT_PADDING_WORDS(4);
            BitField<0, 6, u32> num_interp;
            INSERT_PADDING_WORDS(0xA1C3 - 0xA1B6 - 1);
            ShaderPosFormat shader_pos_format;
            ShaderExportFormat z_export_format;
            ColorExportFormat color_export_format;
            INSERT_PADDING_WORDS(0xA1E0 - 0xA1C3 - 3);
            std::array<BlendControl, NumColorBuffers> blend_control;
            INSERT_PADDING_WORDS(0xA1F9 - 0xA1E0 - 8);
            IndexBufferBase index_base_address;
            INSERT_PADDING_WORDS(1);
            u32 draw_initiator;
            INSERT_PADDING_WORDS(0xA200 - 0xA1F9 - 4);
            DepthControl depth_control;
            INSERT_PADDING_WORDS(1);
            ColorControl color_control;
            DepthBufferControl depth_buffer_control;
            ClipperControl clipper_control;
            PolygonControl polygon_control;
            ViewportControl viewport_control;
            VsOutputControl vs_output_control;
            INSERT_PADDING_WORDS(0xA29E - 0xA207 - 2);
            u32 index_size;
            u32 max_index_size;
            IndexBufferType index_buffer_type;
            INSERT_PADDING_WORDS(0xA2A1 - 0xA29E - 2);
            u32 enable_primitive_id;
            INSERT_PADDING_WORDS(3);
            u32 enable_primitive_restart;
            INSERT_PADDING_WORDS(0xA2A8 - 0xA2A5 - 1);
            u32 vgt_instance_step_rate_0;
            u32 vgt_instance_step_rate_1;
            INSERT_PADDING_WORDS(0xA2D5 - 0xA2A9 - 1);
            ShaderStageEnable stage_enable;
            INSERT_PADDING_WORDS(9);
            PolygonOffset poly_offset;
            INSERT_PADDING_WORDS(0xA2F8 - 0xA2DF - 5);
            AaConfig aa_config;
            INSERT_PADDING_WORDS(0xA318 - 0xA2F8 - 1);
            ColorBuffer color_buffers[NumColorBuffers];
            INSERT_PADDING_WORDS(0xC242 - 0xA390);
            PrimitiveType primitive_type;
            INSERT_PADDING_WORDS(0xC24C - 0xC243);
            u32 num_indices;
            VgtNumInstances num_instances;
        };
        std::array<u32, NumRegs> reg_array{};

        const ShaderProgram* ProgramForStage(u32 index) const {
            switch (index) {
            case 0:
                return &ps_program;
            case 1:
                return &vs_program;
            case 2:
                return &gs_program;
            case 3:
                return &es_program;
            case 4:
                return &hs_program;
            case 5:
                return &ls_program;
            }
            return nullptr;
        }

        void SetDefaults();
    };

    Regs regs{};

    // See for a comment in context reg parsing code
    union CbDbExtent {
        struct {
            u16 width;
            u16 height;
        };
        u32 raw{0u};

        [[nodiscard]] bool Valid() const {
            return raw != 0;
        }
    };
    std::array<CbDbExtent, NumColorBuffers> last_cb_extent{};
    CbDbExtent last_db_extent{};

public:
    Liverpool();
    ~Liverpool();

    void SubmitGfx(std::span<const u32> dcb, std::span<const u32> ccb);
    void SubmitAsc(u32 vqid, std::span<const u32> acb);

    void SubmitDone() noexcept {
        std::scoped_lock lk{submit_mutex};
        mapped_queues[GfxQueueId].ccb_buffer_offset = 0;
        mapped_queues[GfxQueueId].dcb_buffer_offset = 0;
        submit_done = true;
        submit_cv.notify_one();
    }

    void WaitGpuIdle() noexcept {
        std::unique_lock lk{submit_mutex};
        submit_cv.wait(lk, [this] { return num_submits == 0; });
    }

    bool IsGpuIdle() const {
        return num_submits == 0;
    }

    void SetVoPort(Libraries::VideoOut::VideoOutPort* port) {
        vo_port = port;
    }

    void BindRasterizer(Vulkan::Rasterizer* rasterizer_) {
        rasterizer = rasterizer_;
    }

    void SendCommand(Common::UniqueFunction<void>&& func) {
        std::scoped_lock lk{submit_mutex};
        command_queue.emplace(std::move(func));
        ++num_commands;
        submit_cv.notify_one();
    }

    void reserveCopyBufferSpace() {
        GpuQueue& gfx_queue = mapped_queues[GfxQueueId];
        std::scoped_lock<std::mutex> lk(gfx_queue.m_access);

        constexpr size_t GfxReservedSize = 2_MB >> 2;
        gfx_queue.ccb_buffer.reserve(GfxReservedSize);
        gfx_queue.dcb_buffer.reserve(GfxReservedSize);
    }

private:
    struct Task {
        struct promise_type {
            auto get_return_object() {
                Task task{};
                task.handle = std::coroutine_handle<promise_type>::from_promise(*this);
                return task;
            }
            static constexpr std::suspend_always initial_suspend() noexcept {
                // We want the task to be suspended at start
                return {};
            }
            static constexpr std::suspend_always final_suspend() noexcept {
                return {};
            }
            void unhandled_exception() {
                try {
                    std::rethrow_exception(std::current_exception());
                } catch (const std::exception& e) {
                    UNREACHABLE_MSG("Unhandled exception: {}", e.what());
                }
            }
            void return_void() {}
            struct empty {};
            std::suspend_always yield_value(empty&&) {
                return {};
            }
        };

        using Handle = std::coroutine_handle<promise_type>;
        Handle handle;
    };

    std::pair<std::span<const u32>, std::span<const u32>> CopyCmdBuffers(std::span<const u32> dcb,
                                                                         std::span<const u32> ccb);
    Task ProcessGraphics(std::span<const u32> dcb, std::span<const u32> ccb);
    Task ProcessCeUpdate(std::span<const u32> ccb);
    Task ProcessCompute(std::span<const u32> acb, int vqid);

    void Process(std::stop_token stoken);

    struct GpuQueue {
        std::mutex m_access{};
        std::atomic<u32> dcb_buffer_offset;
        std::atomic<u32> ccb_buffer_offset;
        std::vector<u32> dcb_buffer;
        std::vector<u32> ccb_buffer;
        std::queue<Task::Handle> submits{};
        ComputeProgram cs_state{};
        VAddr indirect_args_addr{};
    };
    std::array<GpuQueue, NumTotalQueues> mapped_queues{};

    struct ConstantEngine {
        void Reset() {
            ce_count = 0;
            de_count = 0;
            ce_compare_count = 0;
        }

        [[nodiscard]] u32 Diff() const {
            ASSERT_MSG(ce_count >= de_count, "DE counter is ahead of CE");
            return ce_count - de_count;
        }

        u32 ce_compare_count{};
        u32 ce_count{};
        u32 de_count{};
        static std::array<u8, 48_KB> constants_heap;
    } cblock{};

    Vulkan::Rasterizer* rasterizer{};
    Libraries::VideoOut::VideoOutPort* vo_port{};
    std::jthread process_thread{};
    std::atomic<u32> num_submits{};
    std::atomic<u32> num_commands{};
    std::atomic<bool> submit_done{};
    std::mutex submit_mutex;
    std::condition_variable_any submit_cv;
    std::queue<Common::UniqueFunction<void>> command_queue{};
};

static_assert(GFX6_3D_REG_INDEX(ps_program) == 0x2C08);
static_assert(GFX6_3D_REG_INDEX(vs_program) == 0x2C48);
static_assert(GFX6_3D_REG_INDEX(vs_program.user_data) == 0x2C4C);
static_assert(GFX6_3D_REG_INDEX(gs_program) == 0x2C88);
static_assert(GFX6_3D_REG_INDEX(es_program) == 0x2CC8);
static_assert(GFX6_3D_REG_INDEX(hs_program) == 0x2D08);
static_assert(GFX6_3D_REG_INDEX(ls_program) == 0x2D48);
static_assert(GFX6_3D_REG_INDEX(cs_program) == 0x2E00);
static_assert(GFX6_3D_REG_INDEX(cs_program.dim_z) == 0x2E03);
static_assert(GFX6_3D_REG_INDEX(cs_program.address_lo) == 0x2E0C);
static_assert(GFX6_3D_REG_INDEX(cs_program.user_data) == 0x2E40);
static_assert(GFX6_3D_REG_INDEX(depth_render_control) == 0xA000);
static_assert(GFX6_3D_REG_INDEX(depth_view) == 0xA002);
static_assert(GFX6_3D_REG_INDEX(depth_htile_data_base) == 0xA005);
static_assert(GFX6_3D_REG_INDEX(screen_scissor) == 0xA00C);
static_assert(GFX6_3D_REG_INDEX(depth_buffer.z_info) == 0xA010);
static_assert(GFX6_3D_REG_INDEX(depth_buffer.depth_slice) == 0xA017);
static_assert(GFX6_3D_REG_INDEX(color_target_mask) == 0xA08E);
static_assert(GFX6_3D_REG_INDEX(color_shader_mask) == 0xA08F);
static_assert(GFX6_3D_REG_INDEX(viewport_scissors) == 0xA094);
static_assert(GFX6_3D_REG_INDEX(primitive_restart_index) == 0xA103);
static_assert(GFX6_3D_REG_INDEX(stencil_control) == 0xA10B);
static_assert(GFX6_3D_REG_INDEX(viewports) == 0xA10F);
static_assert(GFX6_3D_REG_INDEX(clip_user_data) == 0xA16F);
static_assert(GFX6_3D_REG_INDEX(ps_inputs) == 0xA191);
static_assert(GFX6_3D_REG_INDEX(vs_output_config) == 0xA1B1);
static_assert(GFX6_3D_REG_INDEX(num_interp) == 0xA1B6);
static_assert(GFX6_3D_REG_INDEX(shader_pos_format) == 0xA1C3);
static_assert(GFX6_3D_REG_INDEX(z_export_format) == 0xA1C4);
static_assert(GFX6_3D_REG_INDEX(color_export_format) == 0xA1C5);
static_assert(GFX6_3D_REG_INDEX(blend_control) == 0xA1E0);
static_assert(GFX6_3D_REG_INDEX(index_base_address) == 0xA1F9);
static_assert(GFX6_3D_REG_INDEX(draw_initiator) == 0xA1FC);
static_assert(GFX6_3D_REG_INDEX(depth_control) == 0xA200);
static_assert(GFX6_3D_REG_INDEX(color_control) == 0xA202);
static_assert(GFX6_3D_REG_INDEX(clipper_control) == 0xA204);
static_assert(GFX6_3D_REG_INDEX(viewport_control) == 0xA206);
static_assert(GFX6_3D_REG_INDEX(vs_output_control) == 0xA207);
static_assert(GFX6_3D_REG_INDEX(index_size) == 0xA29D);
static_assert(GFX6_3D_REG_INDEX(index_buffer_type) == 0xA29F);
static_assert(GFX6_3D_REG_INDEX(enable_primitive_id) == 0xA2A1);
static_assert(GFX6_3D_REG_INDEX(enable_primitive_restart) == 0xA2A5);
static_assert(GFX6_3D_REG_INDEX(vgt_instance_step_rate_0) == 0xA2A8);
static_assert(GFX6_3D_REG_INDEX(vgt_instance_step_rate_1) == 0xA2A9);
static_assert(GFX6_3D_REG_INDEX(stage_enable) == 0xA2D5);
static_assert(GFX6_3D_REG_INDEX(poly_offset) == 0xA2DF);
static_assert(GFX6_3D_REG_INDEX(aa_config) == 0xA2F8);
static_assert(GFX6_3D_REG_INDEX(color_buffers[0].base_address) == 0xA318);
static_assert(GFX6_3D_REG_INDEX(color_buffers[0].pitch) == 0xA319);
static_assert(GFX6_3D_REG_INDEX(color_buffers[0].slice) == 0xA31A);
static_assert(GFX6_3D_REG_INDEX(color_buffers[7].base_address) == 0xA381);
static_assert(GFX6_3D_REG_INDEX(primitive_type) == 0xC242);
static_assert(GFX6_3D_REG_INDEX(num_instances) == 0xC24D);

#undef GFX6_3D_REG_INDEX

} // namespace AmdGpu
