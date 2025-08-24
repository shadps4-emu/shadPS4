// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <condition_variable>
#include <coroutine>
#include <exception>
#include <mutex>
#include <semaphore>
#include <span>
#include <thread>
#include <vector>
#include <queue>

#include "common/assert.h"
#include "common/bit_field.h"
#include "common/polyfill_thread.h"
#include "common/slot_vector.h"
#include "common/types.h"
#include "common/unique_function.h"
#include "shader_recompiler/params.h"
#include "video_core/amdgpu/pixel_format.h"
#include "video_core/amdgpu/tiling.h"
#include "video_core/amdgpu/types.h"

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
    static constexpr u32 NumComputeRings = NumComputePipes * NumQueuesPerPipe;
    static constexpr u32 NumTotalQueues = NumGfxRings + NumComputeRings;
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
            return std::memcmp(signature.data(), signature_ref, sizeof(signature_ref)) == 0;
        }
    };

    static const BinaryInfo& SearchBinaryInfo(const u32* code, size_t search_limit = 0x2000) {
        constexpr u32 token_mov_vcchi = 0xBEEB03FF;

        if (code[0] == token_mov_vcchi) {
            const auto* info = std::bit_cast<const BinaryInfo*>(code + (code[1] + 1) * 2);
            if (info->Valid()) {
                return *info;
            }
        }

        // First instruction is not s_mov_b32 vcc_hi, #imm,
        // which means we cannot get the binary info via said instruction.
        // The easiest solution is to iterate through each dword and break
        // on the first instance of the binary info.
        constexpr size_t signature_size = sizeof(BinaryInfo::signature_ref) / sizeof(u8);
        const u32* end = code + search_limit;

        for (const u32* it = code; it < end; ++it) {
            if (const BinaryInfo* info = std::bit_cast<const BinaryInfo*>(it); info->Valid()) {
                return *info;
            }
        }

        UNREACHABLE_MSG("Shader binary info not found.");
    }

    struct ShaderProgram {
        u32 address_lo;
        BitField<0, 8, u32> address_hi;
        union {
            BitField<0, 6, u64> num_vgprs;
            BitField<6, 4, u64> num_sgprs;
            BitField<10, 2, u64> priority;
            BitField<12, 2, FpRoundMode> fp_round_mode32;
            BitField<14, 2, FpRoundMode> fp_round_mode64;
            BitField<16, 2, FpDenormMode> fp_denorm_mode32;
            BitField<18, 2, FpDenormMode> fp_denorm_mode64;
            BitField<12, 8, u64> float_mode;
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
            const BinaryInfo& bininfo = SearchBinaryInfo(code);
            const u32 num_dwords = bininfo.length / sizeof(u32);
            return std::span{code, num_dwords};
        }

        [[nodiscard]] u32 NumVgprs() const {
            // Each increment allocates 4 registers, where 0 = 4 registers.
            return (settings.num_vgprs + 1) * 4;
        }
    };

    struct HsTessFactorClamp {
        // I've only seen min=0.0, max=1.0 so far.
        // TODO why is max set to 1.0? Makes no sense
        float hs_max_tess;
        float hs_min_tess;
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

        u32 NumWorkgroups() const noexcept {
            return dim_x * dim_y * dim_z;
        }

        bool IsTgidEnabled(u32 i) const noexcept {
            return (settings.tgid_enable.Value() >> i) & 1;
        }

        std::span<const u32> Code() const {
            const u32* code = Address<u32*>();
            const BinaryInfo& bininfo = SearchBinaryInfo(code);
            const u32 num_dwords = bininfo.length / sizeof(u32);
            return std::span{code, num_dwords};
        }
    };

    template <typename Shader>
    static constexpr const BinaryInfo& GetBinaryInfo(const Shader& sh) {
        const auto* code = sh.template Address<u32*>();
        return SearchBinaryInfo(code);
    }

    static constexpr Shader::ShaderParams GetParams(const auto& sh) {
        auto& bininfo = GetBinaryInfo(sh);
        return {
            .user_data = sh.user_data,
            .code = sh.Code(),
            .hash = bininfo.shader_hash,
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

        [[nodiscard]] ShaderExportFormat GetFormat(const u32 buf_idx) const {
            return static_cast<ShaderExportFormat>((raw >> (buf_idx * 4)) & 0xfu);
        }
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

    struct LineControl {
        u32 width_fixed_point;

        float Width() const {
            return static_cast<float>(width_fixed_point) / 8.0;
        }
    };

    struct ModeControl {
        s32 msaa_enable : 1;
        s32 vport_scissor_enable : 1;
        s32 line_stripple_enable : 1;
        s32 send_unlit_stiles_to_pkr : 1;
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

        union ZInfo {
            BitField<0, 2, ZFormat> format;
            BitField<2, 2, u32> num_samples;
            BitField<13, 3, u32> tile_split;
            BitField<20, 3, TileMode> tile_mode_index;
            BitField<23, 4, u32> decompress_on_n_zplanes;
            BitField<27, 1, u32> allow_expclear;
            BitField<28, 1, u32> read_size;
            BitField<29, 1, u32> tile_surface_en;
            BitField<30, 1, u32> clear_disallowed;
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

        size_t GetDepthSliceSize() const {
            ASSERT(z_info.format != ZFormat::Invalid);
            const auto bpe = NumBits() >> 3; // in bytes
            return (depth_slice.tile_max + 1) * 64 * bpe * NumSamples();
        }

        TileMode GetTileMode() const {
            return z_info.tile_mode_index.Value();
        }

        bool IsTiled() const {
            return GetTileMode() != TileMode::DisplayLinearAligned;
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
        BitField<24, 1, u32> dx_linear_attr_clip_enable;
        BitField<26, 1, u32> zclip_near_disable;
        BitField<27, 1, u32> zclip_far_disable;

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
        BitField<20, 1, u32> persp_corr_dis;
        BitField<21, 1, u32> multi_prim_ib_ena;

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

    struct BorderColorBufferBase {
        u32 base_addr_lo;
        BitField<0, 8, u32> base_addr_hi;

        template <typename T = VAddr>
        T Address() const {
            return std::bit_cast<T>(u64(base_addr_hi) << 40 | u64(base_addr_lo) << 8);
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
        struct {
            s16 bottom_right_x;
            s16 bottom_right_y;
        };

        // From AMD spec: 'Negative numbers clamped to 0'
        static s16 Clamp(s16 value) {
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
        union {
            BitField<0, 15, s32> top_left_x;
            BitField<16, 15, s32> top_left_y;
            BitField<31, 1, s32> window_offset_disable;
        };
        struct {
            s16 bottom_right_x;
            s16 bottom_right_y;
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
        BitField<11, 1, u32> perfcounter_ref;
    };

    struct ClipUserData {
        u32 data_x;
        u32 data_y;
        u32 data_z;
        u32 data_w;
    };

    using BlendConstants = std::array<float, 4>;

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

        u32 raw;
        BitField<0, 5, BlendFactor> color_src_factor;
        BitField<5, 3, BlendFunc> color_func;
        BitField<8, 5, BlendFactor> color_dst_factor;
        BitField<16, 5, BlendFactor> alpha_src_factor;
        BitField<21, 3, BlendFunc> alpha_func;
        BitField<24, 5, BlendFactor> alpha_dst_factor;
        BitField<29, 1, u32> separate_alpha_blend;
        BitField<30, 1, u32> enable;
        BitField<31, 1, u32> disable_rop3;

        bool operator==(const BlendControl& other) const {
            return raw == other.raw;
        }
    };

    union ColorControl {
        enum class OperationMode : u32 {
            Disable = 0u,
            Normal = 1u,
            EliminateFastClear = 2u,
            Resolve = 3u,
            Err = 4u,
            FmaskDecompress = 5u,
        };
        enum class LogicOp : u32 {
            Clear = 0x00,
            Nor = 0x11,
            AndInverted = 0x22,
            CopyInverted = 0x33,
            AndReverse = 0x44,
            Invert = 0x55,
            Xor = 0x66,
            Nand = 0x77,
            And = 0x88,
            Equiv = 0x99,
            Noop = 0xAA,
            OrInverted = 0xBB,
            Copy = 0xCC,
            OrReverse = 0xDD,
            Or = 0xEE,
            Set = 0xFF,
        };

        BitField<0, 1, u32> disable_dual_quad;
        BitField<3, 1, u32> degamma_enable;
        BitField<4, 3, OperationMode> mode;
        BitField<16, 8, LogicOp> rop3;
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
        union Color0Info {
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
            BitField<20, 3, u32> blend_opt_dont_rd_dst;
            BitField<23, 3, u32> blend_opt_discard_pixel;
            BitField<26, 1, u32> fmask_compression_disable_ci;
            BitField<27, 1, u32> fmask_compress_1frag_only;
            BitField<28, 1, u32> dcc_enable;
            BitField<29, 2, u32> cmask_addr_type;
            /// Neo-mode only
            BitField<31, 1, u32> alt_tile_mode;

            u32 u32all;
        } info;
        union Color0Attrib {
            BitField<0, 5, TileMode> tile_mode_index;
            BitField<5, 5, u32> fmask_tile_mode_index;
            BitField<10, 2, u32> fmask_bank_height;
            BitField<12, 3, u32> num_samples_log2;
            BitField<15, 2, u32> num_fragments_log2;
            BitField<17, 1, u32> force_dst_alpha_1;

            u32 u32all;
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
            return base_address && info.format != DataFormat::FormatInvalid;
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
            const auto num_bytes_per_element = NumBitsPerBlock(info.format) / 8u;
            const auto slice_size =
                num_bytes_per_element * (slice.tile_max + 1) * 64u * NumSamples();
            return slice_size;
        }

        TileMode GetTileMode() const {
            return info.linear_general ? TileMode::DisplayLinearAligned
                                       : attrib.tile_mode_index.Value();
        }

        bool IsTiled() const {
            return GetTileMode() != TileMode::DisplayLinearAligned;
        }

        [[nodiscard]] DataFormat GetDataFmt() const {
            return RemapDataFormat(info.format);
        }

        [[nodiscard]] NumberFormat GetNumberFmt() const {
            return RemapNumberFormat(GetFixedNumberFormat(), info.format);
        }

        [[nodiscard]] NumberConversion GetNumberConversion() const {
            return MapNumberConversion(GetFixedNumberFormat(), info.format);
        }

        [[nodiscard]] CompMapping Swizzle() const {
            // clang-format off
            static constexpr std::array<std::array<CompMapping, 4>, 4> mrt_swizzles{{
                // Standard
                std::array<CompMapping, 4>{{
                    {.r = CompSwizzle::Red, .g = CompSwizzle::Zero, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                    {.r = CompSwizzle::Red, .g = CompSwizzle::Green, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                    {.r = CompSwizzle::Red, .g = CompSwizzle::Green, .b = CompSwizzle::Blue, .a = CompSwizzle::Zero},
                    {.r = CompSwizzle::Red, .g = CompSwizzle::Green, .b = CompSwizzle::Blue, .a = CompSwizzle::Alpha},
                }},
                // Alternate
                std::array<CompMapping, 4>{{
                    {.r = CompSwizzle::Green, .g = CompSwizzle::Zero, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                    {.r = CompSwizzle::Red, .g = CompSwizzle::Alpha, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                    {.r = CompSwizzle::Red, .g = CompSwizzle::Green, .b = CompSwizzle::Alpha, .a = CompSwizzle::Zero},
                    {.r = CompSwizzle::Blue, .g = CompSwizzle::Green, .b = CompSwizzle::Red, .a = CompSwizzle::Alpha},
                }},
                // StandardReverse
                std::array<CompMapping, 4>{{
                    {.r = CompSwizzle::Blue, .g = CompSwizzle::Zero, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                    {.r = CompSwizzle::Green, .g = CompSwizzle::Red, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                    {.r = CompSwizzle::Blue, .g = CompSwizzle::Green, .b = CompSwizzle::Red, .a = CompSwizzle::Zero},
                    {.r = CompSwizzle::Alpha, .g = CompSwizzle::Blue, .b = CompSwizzle::Green, .a = CompSwizzle::Red},
                }},
                // AlternateReverse
                std::array<CompMapping, 4>{{
                    {.r = CompSwizzle::Alpha, .g = CompSwizzle::Zero, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                    {.r = CompSwizzle::Alpha, .g = CompSwizzle::Red, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                    {.r = CompSwizzle::Alpha, .g = CompSwizzle::Green, .b = CompSwizzle::Red, .a = CompSwizzle::Zero},
                    {.r = CompSwizzle::Alpha, .g = CompSwizzle::Red, .b = CompSwizzle::Green, .a = CompSwizzle::Blue},
                }},
            }};
            // clang-format on
            const auto swap_idx = static_cast<u32>(info.comp_swap.Value());
            const auto components_idx = NumComponents(info.format) - 1;
            const auto mrt_swizzle = mrt_swizzles[swap_idx][components_idx];
            return RemapSwizzle(info.format, mrt_swizzle);
        }

        [[nodiscard]] NumberFormat GetFixedNumberFormat() const {
            // There is a small difference between T# and CB number types, account for it.
            return info.number_type == NumberFormat::SnormNz ? NumberFormat::Srgb
                                                             : info.number_type.Value();
        }
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
        BitField<2, 1, u32> depth_copy;
        BitField<3, 1, u32> stencil_copy;
        BitField<4, 1, u32> resummarize_enable;
        BitField<5, 1, u32> stencil_compress_disable;
        BitField<6, 1, u32> depth_compress_disable;
        BitField<7, 1, u32> copy_centroid;
        BitField<8, 1, u32> copy_sample;
        BitField<9, 1, u32> decompress_enable;
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

    union DepthRenderOverride {
        u32 raw;
        BitField<0, 2, ForceEnable> force_hiz_enable;
        BitField<2, 2, ForceEnable> force_his_enable0;
        BitField<4, 2, ForceEnable> force_his_enable1;
        BitField<6, 1, u32> force_shader_z_order;
        BitField<7, 1, u32> fast_z_disable;
        BitField<8, 1, u32> fast_stencil_disable;
        BitField<9, 1, u32> noop_cull_disable;
        BitField<10, 1, u32> force_color_kill;
        BitField<11, 1, u32> force_z_read;
        BitField<12, 1, u32> force_stencil_read;
        BitField<13, 2, ForceEnable> force_full_z_range;
        BitField<15, 1, u32> force_qc_smask_conflict;
        BitField<16, 1, u32> disable_viewport_clamp;
        BitField<17, 1, u32> ignore_sc_zrange;
        BitField<18, 1, u32> disable_fully_covered;
        BitField<19, 2, ForceSumm> force_z_limit_summ;
        BitField<21, 5, u32> max_tiles_in_dtt;
        BitField<26, 1, u32> disable_tile_rate_tiles;
        BitField<27, 1, u32> force_z_dirty;
        BitField<28, 1, u32> force_stencil_dirty;
        BitField<29, 1, u32> force_z_valid;
        BitField<30, 1, u32> force_stencil_valid;
        BitField<31, 1, u32> preserve_compression;
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
        enum VgtStages : u32 {
            Vs = 0u, // always enabled
            EsGs = 0xB0u,
            LsHs = 0x45u,
        };

        VgtStages raw;
        BitField<0, 2, u32> ls_en;
        BitField<2, 1, u32> hs_en;
        BitField<3, 2, u32> es_en;
        BitField<5, 1, u32> gs_en;
        BitField<6, 2, u32> vs_en;
        BitField<8, 1, u32> dynamic_hs;

        bool IsStageEnabled(u32 stage) const {
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

    union GsInstances {
        u32 raw;
        struct {
            u32 enable : 2;
            u32 count : 6;
        };

        bool IsEnabled() const {
            return enable && count > 0;
        }
    };

    union GsOutPrimitiveType {
        u32 raw;
        struct {
            GsOutputPrimitiveType outprim_type : 6;
            GsOutputPrimitiveType outprim_type1 : 6;
            GsOutputPrimitiveType outprim_type2 : 6;
            GsOutputPrimitiveType outprim_type3 : 6;
            u32 reserved : 3;
            u32 unique_type_per_stream : 1;
        };

        GsOutputPrimitiveType GetPrimitiveType(u32 stream) const {
            if (unique_type_per_stream == 0) {
                return outprim_type;
            }

            switch (stream) {
            case 0:
                return outprim_type;
            case 1:
                return outprim_type1;
            case 2:
                return outprim_type2;
            case 3:
                return outprim_type3;
            default:
                UNREACHABLE();
            }
        }
    };

    union GsMode {
        enum class Mode : u32 {
            Off = 0,
            ScenarioA = 1,
            ScenarioB = 2,
            ScenarioG = 3,
            ScenarioC = 4,
        };

        u32 raw;
        BitField<0, 3, Mode> mode;
        BitField<3, 2, u32> cut_mode;
        BitField<22, 2, u32> onchip;
    };

    union StreamOutControl {
        u32 raw;
        struct {
            u32 offset_update_done : 1;
            u32 : 31;
        };
    };

    union StreamOutConfig {
        u32 raw;
        struct {
            u32 streamout_0_en : 1;
            u32 streamout_1_en : 1;
            u32 streamout_2_en : 1;
            u32 streamout_3_en : 1;
            u32 rast_stream : 3;
            u32 : 1;
            u32 rast_stream_mask : 4;
            u32 : 19;
            u32 use_rast_stream_mask : 1;
        };
    };

    union StreamOutBufferConfig {
        u32 raw;
        struct {
            u32 stream_0_buf_en : 4;
            u32 stream_1_buf_en : 4;
            u32 stream_2_buf_en : 4;
            u32 stream_3_buf_en : 4;
        };
    };

    union LsHsConfig {
        u32 raw;
        BitField<0, 8, u32> num_patches;
        BitField<8, 6, u32> hs_input_control_points;
        BitField<14, 6, u32> hs_output_control_points;
    };

    union TessellationConfig {
        u32 raw;
        BitField<0, 2, TessellationType> type;
        BitField<2, 3, TessellationPartitioning> partitioning;
        BitField<5, 3, TessellationTopology> topology;
    };

    union TessFactorMemoryBase {
        u32 base;

        u64 MemoryBase() const {
            return static_cast<u64>(base) << 8;
        }
    };

    union Eqaa {
        u32 raw;
        BitField<0, 1, u32> max_anchor_samples;
        BitField<4, 3, u32> ps_iter_samples;
        BitField<8, 3, u32> mask_export_num_samples;
        BitField<12, 3, u32> alpha_to_mask_num_samples;
        BitField<16, 1, u32> high_quality_intersections;
        BitField<17, 1, u32> incoherent_eqaa_reads;
        BitField<18, 1, u32> interpolate_comp_z;
        BitField<19, 1, u32> interpolate_src_z;
        BitField<20, 1, u32> static_anchor_associations;
        BitField<21, 1, u32> alpha_to_mask_eqaa_disable;
        BitField<24, 3, u32> overrasterization_amount;
        BitField<27, 1, u32> enable_postz_overrasterization;
    };

    union PsInput {
        u32 raw;
        struct {
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
        };
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
            INSERT_PADDING_WORDS(0x2D48 - 0x2d08 - 20);
            ShaderProgram ls_program;
            INSERT_PADDING_WORDS(0xA4);
            ComputeProgram cs_program; // shadowed by `cs_state` in `mapped_queues`
            INSERT_PADDING_WORDS(0xA008 - 0x2E00 - 80 - 3 - 5);
            DepthRenderControl depth_render_control;
            INSERT_PADDING_WORDS(1);
            DepthView depth_view;
            DepthRenderOverride depth_render_override;
            INSERT_PADDING_WORDS(1);
            Address depth_htile_data_base;
            INSERT_PADDING_WORDS(2);
            float depth_bounds_min;
            float depth_bounds_max;
            u32 stencil_clear;
            float depth_clear;
            Scissor screen_scissor;
            INSERT_PADDING_WORDS(0xA010 - 0xA00C - 2);
            DepthBuffer depth_buffer;
            INSERT_PADDING_WORDS(8);
            BorderColorBufferBase ta_bc_base;
            INSERT_PADDING_WORDS(0xA080 - 0xA020 - 2);
            WindowOffset window_offset;
            ViewportScissor window_scissor;
            INSERT_PADDING_WORDS(0xA08E - 0xA081 - 2);
            ColorBufferMask color_target_mask;
            ColorBufferMask color_shader_mask;
            ViewportScissor generic_scissor;
            INSERT_PADDING_WORDS(2);
            std::array<ViewportScissor, NumViewports> viewport_scissors;
            std::array<ViewportDepth, NumViewports> viewport_depths;
            INSERT_PADDING_WORDS(0xA102 - 0xA0D4);
            u32 index_offset;
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
            INSERT_PADDING_WORDS(1);
            PsInput ps_input_ena;
            PsInput ps_input_addr;
            INSERT_PADDING_WORDS(1);
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
            INSERT_PADDING_WORDS(0xA287 - 0xA207 - 6);
            LineControl line_control;
            INSERT_PADDING_WORDS(4);
            HsTessFactorClamp hs_clamp;
            INSERT_PADDING_WORDS(0xA290 - 0xA287 - 2);
            GsMode vgt_gs_mode;
            INSERT_PADDING_WORDS(1);
            ModeControl mode_control;
            INSERT_PADDING_WORDS(8);
            GsOutPrimitiveType vgt_gs_out_prim_type;
            INSERT_PADDING_WORDS(1);
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
            INSERT_PADDING_WORDS(0xA2AB - 0xA2A9 - 1);
            u32 vgt_esgs_ring_itemsize;
            u32 vgt_gsvs_ring_itemsize;
            INSERT_PADDING_WORDS(0xA2CE - 0xA2AC - 1);
            BitField<0, 11, u32> vgt_gs_max_vert_out;
            INSERT_PADDING_WORDS(0xA2D5 - 0xA2CE - 1);
            ShaderStageEnable stage_enable;
            LsHsConfig ls_hs_config;
            u32 vgt_gs_vert_itemsize[4];
            TessellationConfig tess_config;
            INSERT_PADDING_WORDS(3);
            PolygonOffset poly_offset;
            GsInstances vgt_gs_instance_cnt;
            StreamOutConfig vgt_strmout_config;
            StreamOutBufferConfig vgt_strmout_buffer_config;
            INSERT_PADDING_WORDS(0xA2F8 - 0xA2E6 - 1);
            AaConfig aa_config;
            INSERT_PADDING_WORDS(0xA318 - 0xA2F8 - 1);
            ColorBuffer color_buffers[NumColorBuffers];
            INSERT_PADDING_WORDS(0xC03F - 0xA390);
            StreamOutControl cp_strmout_cntl;
            INSERT_PADDING_WORDS(0xC242 - 0xC040);
            PrimitiveType primitive_type;
            INSERT_PADDING_WORDS(0xC24C - 0xC243);
            u32 num_indices;
            VgtNumInstances num_instances;
            INSERT_PADDING_WORDS(0xC250 - 0xC24D - 1);
            TessFactorMemoryBase vgt_tf_memory_base;
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

        u32 NumSamples() const {
            // It seems that the number of samples > 1 set in the AA config doesn't mean we're
            // always rendering with MSAA, so we need to derive MS ratio from the CB and DB
            // settings.
            u32 num_samples = 1u;
            if (color_control.mode != ColorControl::OperationMode::Disable) {
                for (auto cb = 0u; cb < NumColorBuffers; ++cb) {
                    const auto& col_buf = color_buffers[cb];
                    if (!col_buf) {
                        continue;
                    }
                    num_samples = std::max(num_samples, col_buf.NumSamples());
                }
            }
            if (depth_buffer.DepthValid() || depth_buffer.StencilValid()) {
                num_samples = std::max(num_samples, depth_buffer.NumSamples());
            }
            return num_samples;
        }

        bool IsClipDisabled() const {
            return clipper_control.clip_disable || primitive_type == PrimitiveType::RectList;
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
    void SubmitAsc(u32 gnm_vqid, std::span<const u32> acb);

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

    template <bool wait_done = false>
    void SendCommand(auto&& func) {
        if (std::this_thread::get_id() == gpu_id) {
            return func();
        }
        if constexpr (wait_done) {
            std::binary_semaphore sem{0};
            {
                std::scoped_lock lk{submit_mutex};
                command_queue.emplace([&sem, &func] {
                    func();
                    sem.release();
                });
                ++num_commands;
                submit_cv.notify_one();
            }
            sem.acquire();
        } else {
            std::scoped_lock lk{submit_mutex};
            command_queue.emplace(std::move(func));
            ++num_commands;
            submit_cv.notify_one();
        }
    }

    void ReserveCopyBufferSpace() {
        GpuQueue& gfx_queue = mapped_queues[GfxQueueId];
        std::scoped_lock<std::mutex> lk(gfx_queue.m_access);

        constexpr size_t GfxReservedSize = 2_MB >> 2;
        gfx_queue.ccb_buffer.reserve(GfxReservedSize);
        gfx_queue.dcb_buffer.reserve(GfxReservedSize);
    }

    inline ComputeProgram& GetCsRegs() {
        return mapped_queues[curr_qid].cs_state;
    }

    struct AscQueueInfo {
        static constexpr size_t Pm4BufferSize = 1024;
        VAddr map_addr;
        u32* read_addr;
        u32 ring_size_dw;
        u32 pipe_id;
        std::array<u32, Pm4BufferSize> tmp_packet;
        u32 tmp_dwords;
    };
    Common::SlotVector<AscQueueInfo> asc_queues{};

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
    template <bool is_indirect = false>
    Task ProcessCompute(std::span<const u32> acb, u32 vqid);

    void ProcessCommands();
    void Process(std::stop_token stoken);

    struct GpuQueue {
        std::mutex m_access{};
        std::atomic<u32> dcb_buffer_offset;
        std::atomic<u32> ccb_buffer_offset;
        std::vector<u32> dcb_buffer;
        std::vector<u32> ccb_buffer;
        std::queue<Task::Handle> submits{};
        ComputeProgram cs_state{};
    };
    std::array<GpuQueue, NumTotalQueues> mapped_queues{};
    u32 num_mapped_queues{1u}; // GFX is always available

    VAddr indirect_args_addr{};

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
    std::thread::id gpu_id;
    int curr_qid{-1};
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
static_assert(GFX6_3D_REG_INDEX(ta_bc_base) == 0xA020);
static_assert(GFX6_3D_REG_INDEX(window_offset) == 0xA080);
static_assert(GFX6_3D_REG_INDEX(window_scissor) == 0xA081);
static_assert(GFX6_3D_REG_INDEX(color_target_mask) == 0xA08E);
static_assert(GFX6_3D_REG_INDEX(color_shader_mask) == 0xA08F);
static_assert(GFX6_3D_REG_INDEX(generic_scissor) == 0xA090);
static_assert(GFX6_3D_REG_INDEX(viewport_scissors) == 0xA094);
static_assert(GFX6_3D_REG_INDEX(index_offset) == 0xA102);
static_assert(GFX6_3D_REG_INDEX(primitive_restart_index) == 0xA103);
static_assert(GFX6_3D_REG_INDEX(stencil_control) == 0xA10B);
static_assert(GFX6_3D_REG_INDEX(viewports) == 0xA10F);
static_assert(GFX6_3D_REG_INDEX(clip_user_data) == 0xA16F);
static_assert(GFX6_3D_REG_INDEX(ps_inputs) == 0xA191);
static_assert(GFX6_3D_REG_INDEX(vs_output_config) == 0xA1B1);
static_assert(GFX6_3D_REG_INDEX(ps_input_ena) == 0xA1B3);
static_assert(GFX6_3D_REG_INDEX(ps_input_addr) == 0xA1B4);
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
static_assert(GFX6_3D_REG_INDEX(line_control) == 0xA282);
static_assert(GFX6_3D_REG_INDEX(hs_clamp) == 0xA287);
static_assert(GFX6_3D_REG_INDEX(vgt_gs_mode) == 0xA290);
static_assert(GFX6_3D_REG_INDEX(mode_control) == 0xA292);
static_assert(GFX6_3D_REG_INDEX(vgt_gs_out_prim_type) == 0xA29B);
static_assert(GFX6_3D_REG_INDEX(index_size) == 0xA29D);
static_assert(GFX6_3D_REG_INDEX(index_buffer_type) == 0xA29F);
static_assert(GFX6_3D_REG_INDEX(enable_primitive_id) == 0xA2A1);
static_assert(GFX6_3D_REG_INDEX(enable_primitive_restart) == 0xA2A5);
static_assert(GFX6_3D_REG_INDEX(vgt_instance_step_rate_0) == 0xA2A8);
static_assert(GFX6_3D_REG_INDEX(vgt_instance_step_rate_1) == 0xA2A9);
static_assert(GFX6_3D_REG_INDEX(vgt_esgs_ring_itemsize) == 0xA2AB);
static_assert(GFX6_3D_REG_INDEX(vgt_gsvs_ring_itemsize) == 0xA2AC);
static_assert(GFX6_3D_REG_INDEX(vgt_gs_max_vert_out) == 0xA2CE);
static_assert(GFX6_3D_REG_INDEX(stage_enable) == 0xA2D5);
static_assert(GFX6_3D_REG_INDEX(vgt_gs_vert_itemsize[0]) == 0xA2D7);
static_assert(GFX6_3D_REG_INDEX(tess_config) == 0xA2DB);
static_assert(GFX6_3D_REG_INDEX(poly_offset) == 0xA2DF);
static_assert(GFX6_3D_REG_INDEX(vgt_gs_instance_cnt) == 0xA2E4);
static_assert(GFX6_3D_REG_INDEX(vgt_strmout_config) == 0xA2E5);
static_assert(GFX6_3D_REG_INDEX(vgt_strmout_buffer_config) == 0xA2E6);
static_assert(GFX6_3D_REG_INDEX(aa_config) == 0xA2F8);
static_assert(GFX6_3D_REG_INDEX(color_buffers[0].base_address) == 0xA318);
static_assert(GFX6_3D_REG_INDEX(color_buffers[0].pitch) == 0xA319);
static_assert(GFX6_3D_REG_INDEX(color_buffers[0].slice) == 0xA31A);
static_assert(GFX6_3D_REG_INDEX(color_buffers[7].base_address) == 0xA381);
static_assert(GFX6_3D_REG_INDEX(cp_strmout_cntl) == 0xC03F);
static_assert(GFX6_3D_REG_INDEX(primitive_type) == 0xC242);
static_assert(GFX6_3D_REG_INDEX(num_instances) == 0xC24D);
static_assert(GFX6_3D_REG_INDEX(vgt_tf_memory_base) == 0xc250);

#undef GFX6_3D_REG_INDEX

} // namespace AmdGpu
