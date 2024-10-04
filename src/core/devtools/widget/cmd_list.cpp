//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

// Credits to https://github.com/psucien/tlg-emu-tools/

#include <string>
#include <gcn/si_ci_vi_merged_offset.h>
#include <imgui.h>

#include "cmd_list.h"
#include "frame_dump.h"
#include "imgui_internal.h"
#include "imgui_memory_editor.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/amdgpu/pm4_cmds.h"

#define CONTEXT_SPACE_START 0x0000a000
#define PERSISTENT_SPACE_START 0x00002c00

using namespace ImGui;
using namespace Pal::Gfx6;
using Liverpool = AmdGpu::Liverpool;
using magic_enum::enum_name;

namespace Core::Devtools::Gcn {

const char* GetContextRegName(u32 reg_offset);
const char* GetShaderRegName(u32 reg_offset);
const char* GetOpCodeName(u32 op);

} // namespace Core::Devtools::Gcn

namespace Core::Devtools::Widget {

template <typename HdrType>
static HdrType GetNext(HdrType this_pm4, uint32_t n) {
    HdrType curr_pm4 = this_pm4;
    while (n) {
        curr_pm4 = reinterpret_cast<HdrType>(reinterpret_cast<uint32_t const*>(curr_pm4) +
                                             curr_pm4->count + 2);
        --n;
    }
    return curr_pm4;
}

static void ParsePolygonControl(u32 value) {
    auto const reg = reinterpret_cast<AmdGpu::Liverpool::PolygonControl const&>(value);

    if (BeginTable("PA_SU_SC_MODE_CNTL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("CULL_FRONT");
        TableSetColumnIndex(1);
        Text("%X", reg.cull_front.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("CULL_BACK");
        TableSetColumnIndex(1);
        Text("%X", reg.cull_back.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FACE");
        TableSetColumnIndex(1);
        Text("%s", enum_name(reg.front_face.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("POLY_MODE");
        TableSetColumnIndex(1);
        Text("%X", reg.enable_polygon_mode.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("POLYMODE_FRONT_PTYPE");
        TableSetColumnIndex(1);
        Text("%s", enum_name(reg.polygon_mode_front.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("POLYMODE_BACK_PTYPE");
        TableSetColumnIndex(1);
        Text("%s", enum_name(reg.polygon_mode_back.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("POLY_OFFSET_FRONT_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.enable_polygon_offset_front.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("POLY_OFFSET_BACK_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.enable_polygon_offset_back.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("POLY_OFFSET_PARA_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.enable_polygon_offset_para.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VTX_WINDOW_OFFSET_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.enable_window_offset.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("PROVOKING_VTX_LAST");
        TableSetColumnIndex(1);
        Text("%X (%s)", (u32)reg.provoking_vtx_last.Value(),
             enum_name(reg.provoking_vtx_last.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("PERSP_CORR_DIS");
        TableSetColumnIndex(1);
        Text("%X", reg.persp_corr_dis.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("MULTI_PRIM_IB_ENA");
        TableSetColumnIndex(1);
        Text("%X", reg.multi_prim_ib_ena.Value());

        EndTable();
    }
}

static void ParseAaConfig(u32 value) {
    auto const reg = reinterpret_cast<Liverpool::AaConfig const&>(value);

    if (BeginTable("PA_SC_AA_CONFIG", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("MSAA_NUM_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.msaa_num_samples.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("AA_MASK_CENTROID_DTMN");
        TableSetColumnIndex(1);
        Text("%X", reg.aa_mask_centroid_dtmn.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("MAX_SAMPLE_DIST");
        TableSetColumnIndex(1);
        Text("%X", reg.max_sample_dst.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("MSAA_EXPOSED_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.msaa_exposed_samples.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DETAIL_TO_EXPOSED_MODE");
        TableSetColumnIndex(1);
        Text("%X", reg.detail_to_exposed_mode.Value());

        EndTable();
    }
}

static void ParseViewportControl(u32 value) {
    auto const reg = reinterpret_cast<Liverpool::ViewportControl const&>(value);

    if (BeginTable("PA_CL_VTE_CNTL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("VPORT_X_SCALE_ENA");
        TableSetColumnIndex(1);
        Text("%X", reg.xscale_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VPORT_X_OFFSET_ENA");
        TableSetColumnIndex(1);
        Text("%X", reg.yoffset_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VPORT_Y_SCALE_ENA");
        TableSetColumnIndex(1);
        Text("%X", reg.yscale_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VPORT_Y_OFFSET_ENA");
        TableSetColumnIndex(1);
        Text("%X", reg.yoffset_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VPORT_Z_SCALE_ENA");
        TableSetColumnIndex(1);
        Text("%X", reg.zscale_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VPORT_Z_OFFSET_ENA");
        TableSetColumnIndex(1);
        Text("%X", reg.zoffset_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VTX_XY_FMT");
        TableSetColumnIndex(1);
        Text("%X", reg.xy_transformed.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VTX_Z_FMT");
        TableSetColumnIndex(1);
        Text("%X", reg.z_transformed.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VTX_W0_FMT");
        TableSetColumnIndex(1);
        Text("%X", reg.w_transformed.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("PERFCOUNTER_REF");
        TableSetColumnIndex(1);
        Text("%X", reg.perfcounter_ref.Value());

        EndTable();
    }
}

static void ParseColorControl(u32 value) {
    auto const reg = reinterpret_cast<Liverpool::ColorControl const&>(value);

    if (BeginTable("CB_COLOR_CONTROL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("DISABLE_DUAL_QUAD__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.disable_dual_quad.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DEGAMMA_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.degamma_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("MODE");
        TableSetColumnIndex(1);
        Text("%X (%s)", (u32)reg.mode.Value(), enum_name(reg.mode.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ROP3");
        TableSetColumnIndex(1);
        Text("%X", reg.rop3.Value());

        EndTable();
    }
}

static void ParseColor0Info(u32 value) {
    auto const reg = reinterpret_cast<Liverpool::ColorBuffer::Color0Info const&>(value);

    if (BeginTable("CB_COLOR_INFO", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("ENDIAN");
        TableSetColumnIndex(1);
        Text("%s", enum_name(reg.endian.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FORMAT");
        TableSetColumnIndex(1);
        Text("%s", enum_name(reg.format.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("LINEAR_GENERAL");
        TableSetColumnIndex(1);
        Text("%X", reg.linear_general.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("NUMBER_TYPE");
        TableSetColumnIndex(1);
        Text("%s", enum_name(reg.number_type.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("COMP_SWAP");
        TableSetColumnIndex(1);
        Text("%s", enum_name(reg.comp_swap.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FAST_CLEAR");
        TableSetColumnIndex(1);
        Text("%X", reg.fast_clear.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("COMPRESSION");
        TableSetColumnIndex(1);
        Text("%X", reg.compression.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("BLEND_CLAMP");
        TableSetColumnIndex(1);
        Text("%X", reg.blend_clamp.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("BLEND_BYPASS");
        TableSetColumnIndex(1);
        Text("%X", reg.blend_bypass.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("SIMPLE_FLOAT");
        TableSetColumnIndex(1);
        Text("%X", reg.simple_float.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ROUND_MODE");
        TableSetColumnIndex(1);
        Text("%X (%s)", (u32)reg.round_mode.Value(), enum_name(reg.round_mode.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("CMASK_IS_LINEAR");
        TableSetColumnIndex(1);
        Text("%X", reg.cmask_is_linear.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("BLEND_OPT_DONT_RD_DST");
        TableSetColumnIndex(1);
        Text("%X", reg.blend_opt_dont_rd_dst.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("BLEND_OPT_DISCARD_PIXEL");
        TableSetColumnIndex(1);
        Text("%X", reg.blend_opt_discard_pixel.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FMASK_COMPRESSION_DISABLE__CI__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.fmask_compression_disable_ci.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FMASK_COMPRESS_1FRAG_ONLY__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.fmask_compress_1frag_only.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DCC_ENABLE__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.dcc_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("CMASK_ADDR_TYPE__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.cmask_addr_type.Value());

        EndTable();
    }
}

static void ParseColor0Attrib(u32 value) {
    auto const reg = reinterpret_cast<Liverpool::ColorBuffer::Color0Attrib const&>(value);

    if (BeginTable("CB_COLOR_ATTRIB", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("TILE_MODE_INDEX");
        TableSetColumnIndex(1);
        Text("%s", enum_name(reg.tile_mode_index.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FMASK_TILE_MODE_INDEX");
        TableSetColumnIndex(1);
        Text("%X", reg.fmask_tile_mode_index.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FMASK_BANK_HEIGHT");
        TableSetColumnIndex(1);
        Text("%X", reg.fmask_bank_height.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("NUM_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.num_samples_log2.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("NUM_FRAGMENTS");
        TableSetColumnIndex(1);
        Text("%X", reg.num_fragments_log2.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FORCE_DST_ALPHA_1");
        TableSetColumnIndex(1);
        Text("%X", reg.force_dst_alpha_1.Value());

        EndTable();
    }
}

static void ParseBlendControl(u32 value) {
    auto const reg = reinterpret_cast<Liverpool::BlendControl const&>(value);

    if (BeginTable("CB_BLEND_CONTROL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("COLOR_SRCBLEND");
        TableSetColumnIndex(1);
        Text("%X (%s)", (u32)reg.color_src_factor.Value(),
             enum_name(reg.color_src_factor.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("COLOR_COMB_FCN");
        TableSetColumnIndex(1);
        Text("%X (%s)", (u32)reg.color_func.Value(), enum_name(reg.color_func.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("COLOR_DESTBLEND");
        TableSetColumnIndex(1);
        Text("%X (%s)", (u32)reg.color_dst_factor.Value(),
             enum_name(reg.color_dst_factor.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ALPHA_SRCBLEND");
        TableSetColumnIndex(1);
        Text("%X (%s)", (u32)reg.alpha_src_factor.Value(),
             enum_name(reg.alpha_src_factor.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ALPHA_COMB_FCN");
        TableSetColumnIndex(1);
        Text("%X (%s)", (u32)reg.alpha_func.Value(), enum_name(reg.alpha_func.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ALPHA_DESTBLEND");
        TableSetColumnIndex(1);
        Text("%X (%s)", (u32)reg.alpha_dst_factor.Value(),
             enum_name(reg.alpha_dst_factor.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("SEPARATE_ALPHA_BLEND");
        TableSetColumnIndex(1);
        Text("%X", reg.separate_alpha_blend.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DISABLE_ROP3");
        TableSetColumnIndex(1);
        Text("%X", reg.disable_rop3.Value());

        EndTable();
    }
}

static void ParseDepthRenderControl(u32 value) {
    auto const reg = reinterpret_cast<Liverpool::DepthRenderControl const&>(value);

    if (BeginTable("DB_RENDER_CONTROL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("DEPTH_CLEAR_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.depth_clear_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("STENCIL_CLEAR_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.stencil_clear_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DEPTH_COPY");
        TableSetColumnIndex(1);
        Text("%X", reg.depth_clear_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("STENCIL_COPY");
        TableSetColumnIndex(1);
        Text("%X", reg.stencil_copy.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("RESUMMARIZE_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.resummarize_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("STENCIL_COMPRESS_DISABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.stencil_compress_disable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DEPTH_COMPRESS_DISABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.depth_compress_disable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("COPY_CENTROID");
        TableSetColumnIndex(1);
        Text("%X", reg.copy_centroid.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("COPY_SAMPLE");
        TableSetColumnIndex(1);
        Text("%X", reg.copy_sample.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DECOMPRESS_ENABLE__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.decompress_enable.Value());

        EndTable();
    }
}

static void ParseDepthControl(u32 value) {
    auto const reg = reinterpret_cast<Liverpool::DepthControl const&>(value);

    if (BeginTable("DB_DEPTH_CONTROL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("STENCIL_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.stencil_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("Z_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.depth_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("Z_WRITE_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.depth_write_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DEPTH_BOUNDS_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.depth_bounds_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ZFUNC");
        TableSetColumnIndex(1);
        Text("%X (%s)", (u32)reg.depth_func.Value(), enum_name(reg.depth_func.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("BACKFACE_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.backface_enable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("STENCILFUNC");
        TableSetColumnIndex(1);
        Text("%X (%s)", (u32)reg.stencil_ref_func.Value(),
             enum_name(reg.stencil_ref_func.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("STENCILFUNC_BF");
        TableSetColumnIndex(1);
        Text("%X (%s)", (u32)reg.stencil_bf_func.Value(),
             enum_name(reg.stencil_bf_func.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ENABLE_COLOR_WRITES_ON_DEPTH_FAIL");
        TableSetColumnIndex(1);
        Text("%X", reg.enable_color_writes_on_depth_fail.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DISABLE_COLOR_WRITES_ON_DEPTH_PASS");
        TableSetColumnIndex(1);
        Text("%X", reg.disable_color_writes_on_depth_pass.Value());

        EndTable();
    }
}

static void ParseEqaa(u32 value) {
    auto const reg = reinterpret_cast<Liverpool::Eqaa const&>(value);

    if (BeginTable("DB_DEPTH_CONTROL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("MAX_ANCHOR_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.max_anchor_samples.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("PS_ITER_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.ps_iter_samples.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("MASK_EXPORT_NUM_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.mask_export_num_samples.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ALPHA_TO_MASK_NUM_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.alpha_to_mask_num_samples.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("HIGH_QUALITY_INTERSECTIONS");
        TableSetColumnIndex(1);
        Text("%X", reg.high_quality_intersections.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("INCOHERENT_EQAA_READS");
        TableSetColumnIndex(1);
        Text("%X", reg.incoherent_eqaa_reads.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("INTERPOLATE_COMP_Z");
        TableSetColumnIndex(1);
        Text("%X", reg.interpolate_comp_z.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("INTERPOLATE_SRC_Z");
        TableSetColumnIndex(1);
        Text("%X", reg.interpolate_src_z.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("STATIC_ANCHOR_ASSOCIATIONS");
        TableSetColumnIndex(1);
        Text("%X", reg.static_anchor_associations.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ALPHA_TO_MASK_EQAA_DISABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.alpha_to_mask_eqaa_disable.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("OVERRASTERIZATION_AMOUNT");
        TableSetColumnIndex(1);
        Text("%X", reg.overrasterization_amount.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ENABLE_POSTZ_OVERRASTERIZATION");
        TableSetColumnIndex(1);
        Text("%X", reg.enable_postz_overrasterization.Value());

        EndTable();
    }
}

static void ParseZInfo(u32 value) {
    auto const reg = reinterpret_cast<Liverpool::DepthBuffer::ZInfo const&>(value);

    if (BeginTable("DB_DEPTH_CONTROL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("FORMAT");
        TableSetColumnIndex(1);
        Text("%X (%s)", (u32)reg.format.Value(), enum_name(reg.format.Value()).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("NUM_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.num_samples.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("TILE_SPLIT__CI__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.tile_split.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("TILE_MODE_INDEX");
        TableSetColumnIndex(1);
        Text("%X", reg.tile_mode_index.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DECOMPRESS_ON_N_ZPLANES__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.decompress_on_n_zplanes.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ALLOW_EXPCLEAR");
        TableSetColumnIndex(1);
        Text("%X", reg.allow_expclear.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("READ_SIZE");
        TableSetColumnIndex(1);
        Text("%X", reg.read_size.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("TILE_SURFACE_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.tile_surface_en.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("CLEAR_DISALLOWED__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.clear_disallowed.Value());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ZRANGE_PRECISION");
        TableSetColumnIndex(1);
        Text("%X", reg.zrange_precision.Value());

        EndTable();
    }
}

void CmdListViewer::OnNop(AmdGpu::PM4Type3Header const* header, u32 const* body) {
    using namespace std::string_view_literals;

    enum class NOP_PAYLOAD : u32 {
        ACB_SUBMIT_MRK = 0x68750013,
        ALLOC_ALIGN8 = 0x68753000,
        PUSH_MARKER = 0x68750001,
        SET_VSHARP = 0x68750004,
        SET_TSHARP = 0x68750005,
        SET_SSHARP = 0x68750006,
        SET_USER_DATA = 0x6875000d,
    };
    auto get_noppayload_text = [](NOP_PAYLOAD const nop_payload) {
        switch (nop_payload) {
        case NOP_PAYLOAD::ACB_SUBMIT_MRK:
            return "ACB_SUBMIT_MRK"sv;
        case NOP_PAYLOAD::ALLOC_ALIGN8:
            return "ALLOC_ALIGN8"sv;
        case NOP_PAYLOAD::PUSH_MARKER:
            return "PUSH_MARKER"sv;
        case NOP_PAYLOAD::SET_VSHARP:
            return "SET_VSHARP"sv;
        case NOP_PAYLOAD::SET_TSHARP:
            return "SET_TSHARP"sv;
        case NOP_PAYLOAD::SET_SSHARP:
            return "SET_SSHARP"sv;
        case NOP_PAYLOAD::SET_USER_DATA:
            return "SET_USER_DATA"sv;
        }
        return ""sv;
    };

    Separator();
    BeginGroup();

    auto const* pkt = reinterpret_cast<AmdGpu::PM4CmdNop const*>(header);
    auto const* payload = &body[0];

    // Dump payload
    for (unsigned i = 0; i < pkt->header.count + 1; ++i) {
        Text("%02X: %08X", i, payload[i]);
        if ((payload[i] & 0xffff0000) == 0x68750000) {
            const auto& e = get_noppayload_text((NOP_PAYLOAD)payload[i]);
            if (!e.empty()) {
                SameLine();
                Text("(%s)", e.data());
            }
        }
    }

    EndGroup();
}
void CmdListViewer::OnSetBase(AmdGpu::PM4Type3Header const* header, u32 const* body) {
    Separator();
    BeginGroup();

    auto const* pkt = reinterpret_cast<AmdGpu::PM4CmdSetBase const*>(header);
    Text("BASE_INDEX: %08X", body[0]);
    Text("ADDRESS0  : %08X", body[1]);
    Text("ADDRESS1  : %08X", body[2]);

    EndGroup();
}

void CmdListViewer::OnSetContextReg(AmdGpu::PM4Type3Header const* header, u32 const* body) {
    Separator();
    BeginGroup();

    auto const* pkt = reinterpret_cast<AmdGpu::PM4CmdSetData const*>(header);

    for (auto i = 0u; i < header->count; ++i) {
        auto const absOffset = CONTEXT_SPACE_START + pkt->reg_offset + i;
        Text("reg: %4X (%s)", pkt->reg_offset + i, Gcn::GetContextRegName(absOffset));
        Text("[%08X]", body[i + 1]);

        switch (absOffset) {
        case mmPA_SU_SC_MODE_CNTL: {
            if (IsItemHovered() && BeginTooltip()) {
                ParsePolygonControl(body[1]);
                EndTooltip();
            }
            break;
        }
        case mmPA_SC_AA_CONFIG: {
            if (IsItemHovered() && BeginTooltip()) {
                ParseAaConfig(body[1]);
                EndTooltip();
            }
            break;
        }
        case mmPA_CL_VTE_CNTL: {
            if (IsItemHovered() && BeginTooltip()) {
                ParseViewportControl(body[1]);
                EndTooltip();
            }
            break;
        }
        case mmCB_COLOR_CONTROL:
            if (IsItemHovered() && BeginTooltip()) {
                ParseColorControl(body[1]);
                EndTooltip();
            }
            break;
        case mmCB_COLOR0_INFO:
            [[fallthrough]];
        case mmCB_COLOR1_INFO:
            [[fallthrough]];
        case mmCB_COLOR2_INFO:
            [[fallthrough]];
        case mmCB_COLOR3_INFO:
            [[fallthrough]];
        case mmCB_COLOR4_INFO:
            [[fallthrough]];
        case mmCB_COLOR5_INFO:
            [[fallthrough]];
        case mmCB_COLOR6_INFO:
            [[fallthrough]];
        case mmCB_COLOR7_INFO: {
            if (IsItemHovered() && BeginTooltip()) {
                ParseColor0Info(body[i + 1]);
                EndTooltip();
            }
            break;
        }
        case mmCB_COLOR0_ATTRIB:
            [[fallthrough]];
        case mmCB_COLOR1_ATTRIB:
            [[fallthrough]];
        case mmCB_COLOR2_ATTRIB:
            [[fallthrough]];
        case mmCB_COLOR3_ATTRIB:
            [[fallthrough]];
        case mmCB_COLOR4_ATTRIB:
            [[fallthrough]];
        case mmCB_COLOR5_ATTRIB:
            [[fallthrough]];
        case mmCB_COLOR6_ATTRIB:
            [[fallthrough]];
        case mmCB_COLOR7_ATTRIB: {
            if (IsItemHovered() && BeginTooltip()) {
                ParseColor0Attrib(body[i + 1]);
                EndTooltip();
            }
            break;
        }
        case mmCB_BLEND0_CONTROL:
            [[fallthrough]];
        case mmCB_BLEND1_CONTROL:
            [[fallthrough]];
        case mmCB_BLEND2_CONTROL:
            [[fallthrough]];
        case mmCB_BLEND3_CONTROL:
            [[fallthrough]];
        case mmCB_BLEND4_CONTROL:
            [[fallthrough]];
        case mmCB_BLEND5_CONTROL:
            [[fallthrough]];
        case mmCB_BLEND6_CONTROL:
            [[fallthrough]];
        case mmCB_BLEND7_CONTROL: {
            if (IsItemHovered() && BeginTooltip()) {
                ParseBlendControl(body[i + 1]);
                EndTooltip();
            }
            break;
        }
        case mmDB_RENDER_CONTROL: {
            if (IsItemHovered() && BeginTooltip()) {
                ParseDepthRenderControl(body[1]);
                EndTooltip();
            }
            break;
        }
        case mmDB_DEPTH_CONTROL: {
            if (IsItemHovered() && BeginTooltip()) {
                ParseDepthControl(body[1]);
                EndTooltip();
            }
            break;
        }
        case mmDB_EQAA: {
            if (IsItemHovered() && BeginTooltip()) {
                ParseEqaa(body[1]);
                EndTooltip();
            }
            break;
        }
        case mmDB_Z_INFO: {
            if (IsItemHovered() && BeginTooltip()) {
                ParseZInfo(body[1]);
                EndTooltip();
            }
            break;
        }
        case mmDB_HTILE_DATA_BASE: {
            auto const& reg = reinterpret_cast<u32 const&>(body[1]);
            Text("addr: %08x", reg * 256u);
            break;
        }
        default:
            break;
        }

        //...
    }

    EndGroup();
}

void CmdListViewer::OnSetShReg(AmdGpu::PM4Type3Header const* header, u32 const* body) {
    Separator();
    BeginGroup();

    auto const* pkt = reinterpret_cast<AmdGpu::PM4CmdSetData const*>(header);

    for (auto i = 0u; i < header->count; ++i) {
        auto const absOffset = PERSISTENT_SPACE_START + pkt->reg_offset + i;
        Text("reg: %4X (%s)", pkt->reg_offset + i, Gcn::GetShaderRegName(absOffset));

        Text("%08X", *((uint32_t*)header + 2 + i));

        //...
    }

    EndGroup();
}

void CmdListViewer::OnDispatch(AmdGpu::PM4Type3Header const* header, u32 const* body) {
    Separator();
    BeginGroup();

    auto const* pkt = reinterpret_cast<AmdGpu::PM4CmdDispatchDirect const*>(header);

    Text("DIM_X    : %d", pkt->dim_x);
    Text("DIM_Y    : %d", pkt->dim_y);
    Text("DIM_Z    : %d", pkt->dim_z);
    Text("INITIATOR: %X", pkt->dispatch_initiator);
    if (IsItemHovered() && BeginTooltip()) {
        // TODO: dump_reg
        EndTooltip();
    }

    EndGroup();
}

CmdListViewer::CmdListViewer(FrameDumpViewer* parent, const std::vector<u32>& cmd_list)
    : parent(parent) {
    using namespace AmdGpu;

    cmdb_addr = (uintptr_t)cmd_list.data();
    cmdb_size = cmd_list.size() * sizeof(u32);

    auto const* pm4_hdr = reinterpret_cast<PM4Header const*>(cmdb_addr);

    size_t processed_size = 0;
    size_t prev_offset = 0;

    std::string marker{};

    while (processed_size < cmdb_size) {
        auto* next_pm4_hdr = GetNext(pm4_hdr, 1);
        auto processed_len =
            reinterpret_cast<uintptr_t>(next_pm4_hdr) - reinterpret_cast<uintptr_t>(pm4_hdr);
        processed_size += processed_len;

        if (pm4_hdr->type == PM4Type3Header::TYPE) {

            auto const* pm4_t3 = reinterpret_cast<PM4Type3Header const*>(pm4_hdr);

            if (pm4_t3->opcode == PM4ItOpcode::Nop) {
                auto const* it_body = reinterpret_cast<uint32_t const*>(pm4_hdr + 1);
                if (it_body[0] == 0x68750001) {
                    marker = std::string{(char*)&it_body[1]};
                }
            }

            if (pm4_t3->opcode == PM4ItOpcode::DispatchDirect ||
                pm4_t3->opcode == PM4ItOpcode::DispatchIndirect ||
                pm4_t3->opcode == PM4ItOpcode::DrawIndex2 ||
                pm4_t3->opcode == PM4ItOpcode::DrawIndexAuto ||
                pm4_t3->opcode == PM4ItOpcode::DrawIndexOffset2 ||
                pm4_t3->opcode == PM4ItOpcode::DrawIndexIndirect
                // ...
            ) {
                // All these commands are terminated by NOP at the end, so
                // it is safe to skip it to be even with CP
                // next_pm4_hdr = get_next(next_pm4_hdr, 1);
                // auto constexpr nop_len = 0x10;
                // processed_len += nop_len;
                // processed_size += nop_len;

                batches.emplace_back(BatchInfo{
                    marker,
                    prev_offset,
                    processed_size,
                    processed_size - processed_len,
                    pm4_t3->opcode,
                });
                prev_offset = processed_size;
                marker.clear();
            }
        }

        pm4_hdr = next_pm4_hdr;
    }

    // state batch (last)
    if (processed_size - prev_offset > 0) {
        batches.emplace_back(BatchInfo{
            marker,
            prev_offset,
            processed_size,
            0,
            static_cast<PM4ItOpcode>(0xFF),
        });
    }
}

void CmdListViewer::Draw() {
    if (BeginChild("cmd_queue", {})) {
        char queue_name[32]{};
        if (vqid < 254) {
            std::snprintf(queue_name, sizeof(queue_name), "%s %d", vqid > 254 ? "GFX" : "ASC",
                          vqid);
        } else {
            std::snprintf(queue_name, sizeof(queue_name), "%s", vqid > 254 ? "GFX" : "ASC");
        }

        Text("queue    : %s", queue_name);
        Text("base addr: %08llX", cmdb_addr);
        SameLine();
        if (SmallButton(">")) {
            parent->cmdb_view.Open ^= true;
        }
        Text("size     : %04llX", cmdb_size);
        Separator();

        char batch_hdr[128];
        for (int batch_id = 0; batch_id < batches.size(); ++batch_id) {
            auto processed_size = 0ull;
            auto const* pm4_hdr =
                reinterpret_cast<PM4Header const*>(cmdb_addr + batches[batch_id].start_addr);

            sprintf(batch_hdr, "%08llX: batch-%03d | %s", cmdb_addr + batches[batch_id].start_addr,
                    batch_id, batches[batch_id].marker.c_str());

            if (batch_id == batch_bp) { // highlight batch at breakpoint
                PushStyleColor(ImGuiCol_Header, ImVec4{1.0f, 0.5f, 0.5f, 0.5f});
            }

            if (batches[batch_id].type == static_cast<AmdGpu::PM4ItOpcode>(0xFF) ||
                CollapsingHeader(batch_hdr)) {
                auto const batch_sz = batches[batch_id].end_addr - batches[batch_id].start_addr;
                while (processed_size < batch_sz) {
                    AmdGpu::PM4ItOpcode op{0xFFu};

                    if (pm4_hdr->type == AmdGpu::PM4Type3Header::TYPE) {
                        auto const* pm4_t3 =
                            reinterpret_cast<AmdGpu::PM4Type3Header const*>(pm4_hdr);
                        op = pm4_t3->opcode;

                        static char header_name[128];
                        sprintf(header_name, "%08llX: %s",
                                cmdb_addr + batches[batch_id].start_addr + processed_size,
                                Gcn::GetOpCodeName((u32)op));

                        if (TreeNode(header_name)) {
                            bool just_opened = IsItemToggledOpen();
                            if (BeginTable("split", 1)) {
                                TableNextColumn();
                                Text("size: %d", pm4_hdr->count + 1);

                                if (just_opened) {
                                    // Editor
                                    parent->cmdb_view.GotoAddrAndHighlight(
                                        reinterpret_cast<size_t>(pm4_hdr) - cmdb_addr,
                                        reinterpret_cast<size_t>(pm4_hdr) - cmdb_addr +
                                            (pm4_hdr->count + 2) * 4);
                                }

                                auto const* it_body =
                                    reinterpret_cast<uint32_t const*>(pm4_hdr + 1);

                                switch (op) {
                                case AmdGpu::PM4ItOpcode::Nop: {
                                    OnNop(pm4_t3, it_body);
                                    break;
                                }
                                case AmdGpu::PM4ItOpcode::SetBase: {
                                    OnSetBase(pm4_t3, it_body);
                                    break;
                                }
                                case AmdGpu::PM4ItOpcode::SetContextReg: {
                                    OnSetContextReg(pm4_t3, it_body);
                                    break;
                                }
                                case AmdGpu::PM4ItOpcode::SetShReg: {
                                    OnSetShReg(pm4_t3, it_body);
                                    break;
                                }
                                case AmdGpu::PM4ItOpcode::DispatchDirect: {
                                    OnDispatch(pm4_t3, it_body);
                                    break;
                                }
                                default: {
                                    auto const* payload = &it_body[0];
                                    for (unsigned i = 0; i < pm4_hdr->count + 1; ++i) {
                                        Text("%02X: %08X", i, payload[i]);
                                    }
                                }
                                }

                                EndTable();
                            }
                            TreePop();
                        }
                    } else {
                        Text("<UNK PACKET>");
                    }

                    auto const* next_pm4_hdr = GetNext(pm4_hdr, 1);
                    auto const processed_len = reinterpret_cast<uintptr_t>(next_pm4_hdr) -
                                               reinterpret_cast<uintptr_t>(pm4_hdr);
                    pm4_hdr = next_pm4_hdr;
                    processed_size += processed_len;
                }
            }

            if (batch_id == batch_bp) {
                PopStyleColor();
            }

            if (batch_id == batches.size() - 2) {
                Separator();
            }
        }
    }
    EndChild();
}

} // namespace Core::Devtools::Widget