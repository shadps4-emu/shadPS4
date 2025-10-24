//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

// Credits to https://github.com/psucien/tlg-emu-tools/

#include <cinttypes>
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

static bool group_batches = true;
static bool show_markers = false;

void CmdListViewer::LoadConfig(const char* line) {
    int i;
    if (sscanf(line, "group_batches=%d", &i) == 1) {
        group_batches = i != 0;
        return;
    }
    if (sscanf(line, "show_markers=%d", &i) == 1) {
        show_markers = i != 0;
        return;
    }
}

void CmdListViewer::SerializeConfig(ImGuiTextBuffer* buf) {
    buf->appendf("group_batches=%d\n", group_batches);
    buf->appendf("show_markers=%d\n", show_markers);
}

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

void ParsePolygonControl(u32 value, bool begin_table) {
    auto const reg = reinterpret_cast<AmdGpu::PolygonControl const&>(value);

    if (!begin_table ||
        BeginTable("PA_SU_SC_MODE_CNTL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("CULL_FRONT");
        TableSetColumnIndex(1);
        Text("%X", reg.cull_front);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("CULL_BACK");
        TableSetColumnIndex(1);
        Text("%X", reg.cull_back);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FACE");
        TableSetColumnIndex(1);
        Text("%s", enum_name(reg.front_face).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("POLY_MODE");
        TableSetColumnIndex(1);
        Text("%X", reg.enable_polygon_mode);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("POLYMODE_FRONT_PTYPE");
        TableSetColumnIndex(1);
        Text("%s", enum_name(reg.polygon_mode_front).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("POLYMODE_BACK_PTYPE");
        TableSetColumnIndex(1);
        Text("%s", enum_name(reg.polygon_mode_back).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("POLY_OFFSET_FRONT_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.enable_polygon_offset_front);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("POLY_OFFSET_BACK_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.enable_polygon_offset_back);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("POLY_OFFSET_PARA_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.enable_polygon_offset_para);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VTX_WINDOW_OFFSET_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.enable_window_offset);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("PROVOKING_VTX_LAST");
        TableSetColumnIndex(1);
        Text("%X (%s)", static_cast<u32>(reg.provoking_vtx_last),
             enum_name(reg.provoking_vtx_last).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("PERSP_CORR_DIS");
        TableSetColumnIndex(1);
        Text("%X", reg.persp_corr_dis);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("MULTI_PRIM_IB_ENA");
        TableSetColumnIndex(1);
        Text("%X", reg.multi_prim_ib_ena);

        if (begin_table) {
            EndTable();
        }
    }
}

void ParseAaConfig(u32 value, bool begin_table) {
    auto const reg = reinterpret_cast<AmdGpu::AaConfig const&>(value);

    if (!begin_table ||
        BeginTable("PA_SC_AA_CONFIG", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("MSAA_NUM_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.msaa_num_samples);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("AA_MASK_CENTROID_DTMN");
        TableSetColumnIndex(1);
        Text("%X", reg.aa_mask_centroid_dtmn);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("MAX_SAMPLE_DIST");
        TableSetColumnIndex(1);
        Text("%X", reg.max_sample_dst);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("MSAA_EXPOSED_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.msaa_exposed_samples);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DETAIL_TO_EXPOSED_MODE");
        TableSetColumnIndex(1);
        Text("%X", reg.detail_to_exposed_mode);

        if (begin_table) {
            EndTable();
        }
    }
}

void ParseViewportControl(u32 value, bool begin_table) {
    auto const reg = reinterpret_cast<AmdGpu::ViewportControl const&>(value);

    if (!begin_table ||
        BeginTable("PA_CL_VTE_CNTL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("VPORT_X_SCALE_ENA");
        TableSetColumnIndex(1);
        Text("%X", reg.xscale_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VPORT_X_OFFSET_ENA");
        TableSetColumnIndex(1);
        Text("%X", reg.yoffset_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VPORT_Y_SCALE_ENA");
        TableSetColumnIndex(1);
        Text("%X", reg.yscale_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VPORT_Y_OFFSET_ENA");
        TableSetColumnIndex(1);
        Text("%X", reg.yoffset_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VPORT_Z_SCALE_ENA");
        TableSetColumnIndex(1);
        Text("%X", reg.zscale_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VPORT_Z_OFFSET_ENA");
        TableSetColumnIndex(1);
        Text("%X", reg.zoffset_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VTX_XY_FMT");
        TableSetColumnIndex(1);
        Text("%X", reg.xy_transformed);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VTX_Z_FMT");
        TableSetColumnIndex(1);
        Text("%X", reg.z_transformed);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("VTX_W0_FMT");
        TableSetColumnIndex(1);
        Text("%X", reg.w_transformed);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("PERFCOUNTER_REF");
        TableSetColumnIndex(1);
        Text("%X", reg.perfcounter_ref);

        if (begin_table) {
            EndTable();
        }
    }
}

void ParseColorControl(u32 value, bool begin_table) {
    auto const reg = reinterpret_cast<AmdGpu::ColorControl const&>(value);

    if (!begin_table ||
        BeginTable("CB_COLOR_CONTROL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("DISABLE_DUAL_QUAD__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.disable_dual_quad);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DEGAMMA_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.degamma_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("MODE");
        TableSetColumnIndex(1);
        Text("%X (%s)", static_cast<u32>(reg.mode), enum_name(reg.mode).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ROP3");
        TableSetColumnIndex(1);
        Text("%X", static_cast<u32>(reg.rop3));

        if (begin_table) {
            EndTable();
        }
    }
}

void ParseColor0Info(u32 value, bool begin_table) {
    auto const reg = reinterpret_cast<AmdGpu::ColorBuffer::Color0Info const&>(value);

    if (!begin_table ||
        BeginTable("CB_COLOR_INFO", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("ENDIAN");
        TableSetColumnIndex(1);
        Text("%s", enum_name(reg.endian).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FORMAT");
        TableSetColumnIndex(1);
        Text("%s", enum_name(AmdGpu::DataFormat(reg.format)).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("LINEAR_GENERAL");
        TableSetColumnIndex(1);
        Text("%X", reg.linear_general);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("NUMBER_TYPE");
        TableSetColumnIndex(1);
        Text("%s", enum_name(AmdGpu::NumberFormat(reg.number_type)).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("COMP_SWAP");
        TableSetColumnIndex(1);
        Text("%s", enum_name(reg.comp_swap).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FAST_CLEAR");
        TableSetColumnIndex(1);
        Text("%X", reg.fast_clear);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("COMPRESSION");
        TableSetColumnIndex(1);
        Text("%X", reg.compression);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("BLEND_CLAMP");
        TableSetColumnIndex(1);
        Text("%X", reg.blend_clamp);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("BLEND_BYPASS");
        TableSetColumnIndex(1);
        Text("%X", reg.blend_bypass);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("SIMPLE_FLOAT");
        TableSetColumnIndex(1);
        Text("%X", reg.simple_float);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ROUND_MODE");
        TableSetColumnIndex(1);
        Text("%X (%s)", static_cast<u32>(reg.round_mode), enum_name(reg.round_mode).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("CMASK_IS_LINEAR");
        TableSetColumnIndex(1);
        Text("%X", reg.cmask_is_linear);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("BLEND_OPT_DONT_RD_DST");
        TableSetColumnIndex(1);
        Text("%X", reg.blend_opt_dont_rd_dst);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("BLEND_OPT_DISCARD_PIXEL");
        TableSetColumnIndex(1);
        Text("%X", reg.blend_opt_discard_pixel);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FMASK_COMPRESSION_DISABLE__CI__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.fmask_compression_disable_ci);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FMASK_COMPRESS_1FRAG_ONLY__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.fmask_compress_1frag_only);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DCC_ENABLE__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.dcc_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("CMASK_ADDR_TYPE__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.cmask_addr_type);

        if (begin_table) {
            EndTable();
        }
    }
}

void ParseColor0Attrib(u32 value, bool begin_table) {
    auto const reg = reinterpret_cast<AmdGpu::ColorBuffer::Color0Attrib const&>(value);

    if (!begin_table ||
        BeginTable("CB_COLOR_ATTRIB", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("TILE_MODE_INDEX");
        TableSetColumnIndex(1);
        Text("%s", enum_name(reg.tile_mode_index).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FMASK_TILE_MODE_INDEX");
        TableSetColumnIndex(1);
        Text("%X", reg.fmask_tile_mode_index);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FMASK_BANK_HEIGHT");
        TableSetColumnIndex(1);
        Text("%X", reg.fmask_bank_height);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("NUM_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.num_samples_log2);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("NUM_FRAGMENTS");
        TableSetColumnIndex(1);
        Text("%X", reg.num_fragments_log2);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("FORCE_DST_ALPHA_1");
        TableSetColumnIndex(1);
        Text("%X", reg.force_dst_alpha_1);

        if (begin_table) {
            EndTable();
        }
    }
}

void ParseBlendControl(u32 value, bool begin_table) {
    auto const reg = reinterpret_cast<AmdGpu::BlendControl const&>(value);

    if (!begin_table ||
        BeginTable("CB_BLEND_CONTROL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("COLOR_SRCBLEND");
        TableSetColumnIndex(1);
        Text("%X (%s)", static_cast<u32>(reg.color_src_factor),
             enum_name(reg.color_src_factor).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("COLOR_COMB_FCN");
        TableSetColumnIndex(1);
        Text("%X (%s)", static_cast<u32>(reg.color_func), enum_name(reg.color_func).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("COLOR_DESTBLEND");
        TableSetColumnIndex(1);
        Text("%X (%s)", static_cast<u32>(reg.color_dst_factor),
             enum_name(reg.color_dst_factor).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ALPHA_SRCBLEND");
        TableSetColumnIndex(1);
        Text("%X (%s)", static_cast<u32>(reg.alpha_src_factor),
             enum_name(reg.alpha_src_factor).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ALPHA_COMB_FCN");
        TableSetColumnIndex(1);
        Text("%X (%s)", static_cast<u32>(reg.alpha_func), enum_name(reg.alpha_func).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ALPHA_DESTBLEND");
        TableSetColumnIndex(1);
        Text("%X (%s)", static_cast<u32>(reg.alpha_dst_factor),
             enum_name(reg.alpha_dst_factor).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("SEPARATE_ALPHA_BLEND");
        TableSetColumnIndex(1);
        Text("%X", reg.separate_alpha_blend);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DISABLE_ROP3");
        TableSetColumnIndex(1);
        Text("%X", reg.disable_rop3);

        if (begin_table) {
            EndTable();
        }
    }
}

void ParseDepthRenderControl(u32 value, bool begin_table) {
    auto const reg = reinterpret_cast<AmdGpu::DepthRenderControl const&>(value);

    if (!begin_table ||
        BeginTable("DB_RENDER_CONTROL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("DEPTH_CLEAR_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.depth_clear_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("STENCIL_CLEAR_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.stencil_clear_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DEPTH_COPY");
        TableSetColumnIndex(1);
        Text("%X", reg.depth_clear_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("STENCIL_COPY");
        TableSetColumnIndex(1);
        Text("%X", reg.stencil_copy);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("RESUMMARIZE_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.resummarize_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("STENCIL_COMPRESS_DISABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.stencil_compress_disable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DEPTH_COMPRESS_DISABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.depth_compress_disable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("COPY_CENTROID");
        TableSetColumnIndex(1);
        Text("%X", reg.copy_centroid);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("COPY_SAMPLE");
        TableSetColumnIndex(1);
        Text("%X", reg.copy_sample);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DECOMPRESS_ENABLE__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.decompress_enable);

        if (begin_table) {
            EndTable();
        }
    }
}

void ParseDepthControl(u32 value, bool begin_table) {
    auto const reg = reinterpret_cast<AmdGpu::DepthControl const&>(value);

    if (!begin_table ||
        BeginTable("DB_DEPTH_CONTROL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("STENCIL_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.stencil_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("Z_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.depth_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("Z_WRITE_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.depth_write_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DEPTH_BOUNDS_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.depth_bounds_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ZFUNC");
        TableSetColumnIndex(1);
        Text("%X (%s)", static_cast<u32>(reg.depth_func), enum_name(reg.depth_func).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("BACKFACE_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.backface_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("STENCILFUNC");
        TableSetColumnIndex(1);
        Text("%X (%s)", static_cast<u32>(reg.stencil_ref_func),
             enum_name(reg.stencil_ref_func).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("STENCILFUNC_BF");
        TableSetColumnIndex(1);
        Text("%X (%s)", static_cast<u32>(reg.stencil_bf_func),
             enum_name(reg.stencil_bf_func).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ENABLE_COLOR_WRITES_ON_DEPTH_FAIL");
        TableSetColumnIndex(1);
        Text("%X", reg.enable_color_writes_on_depth_fail);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DISABLE_COLOR_WRITES_ON_DEPTH_PASS");
        TableSetColumnIndex(1);
        Text("%X", reg.disable_color_writes_on_depth_pass);

        if (begin_table) {
            EndTable();
        }
    }
}

void ParseEqaa(u32 value, bool begin_table) {
    auto const reg = reinterpret_cast<AmdGpu::Eqaa const&>(value);

    if (!begin_table ||
        BeginTable("DB_DEPTH_CONTROL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("MAX_ANCHOR_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.max_anchor_samples);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("PS_ITER_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.ps_iter_samples);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("MASK_EXPORT_NUM_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.mask_export_num_samples);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ALPHA_TO_MASK_NUM_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.alpha_to_mask_num_samples);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("HIGH_QUALITY_INTERSECTIONS");
        TableSetColumnIndex(1);
        Text("%X", reg.high_quality_intersections);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("INCOHERENT_EQAA_READS");
        TableSetColumnIndex(1);
        Text("%X", reg.incoherent_eqaa_reads);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("INTERPOLATE_COMP_Z");
        TableSetColumnIndex(1);
        Text("%X", reg.interpolate_comp_z);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("INTERPOLATE_SRC_Z");
        TableSetColumnIndex(1);
        Text("%X", reg.interpolate_src_z);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("STATIC_ANCHOR_ASSOCIATIONS");
        TableSetColumnIndex(1);
        Text("%X", reg.static_anchor_associations);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ALPHA_TO_MASK_EQAA_DISABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.alpha_to_mask_eqaa_disable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("OVERRASTERIZATION_AMOUNT");
        TableSetColumnIndex(1);
        Text("%X", reg.overrasterization_amount);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ENABLE_POSTZ_OVERRASTERIZATION");
        TableSetColumnIndex(1);
        Text("%X", reg.enable_postz_overrasterization);

        if (begin_table) {
            EndTable();
        }
    }
}

void ParseZInfo(u32 value, bool begin_table) {
    auto const reg = reinterpret_cast<AmdGpu::DepthBuffer::ZInfo const&>(value);

    if (!begin_table ||
        BeginTable("DB_DEPTH_CONTROL", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        TableNextRow();
        TableSetColumnIndex(0);
        Text("FORMAT");
        TableSetColumnIndex(1);
        Text("%X (%s)", static_cast<u32>(reg.format), enum_name(reg.format).data());

        TableNextRow();
        TableSetColumnIndex(0);
        Text("NUM_SAMPLES");
        TableSetColumnIndex(1);
        Text("%X", reg.num_samples);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("TILE_SPLIT__CI__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.tile_split);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("TILE_MODE_INDEX");
        TableSetColumnIndex(1);
        Text("%X", static_cast<u32>(reg.tile_mode_index));

        TableNextRow();
        TableSetColumnIndex(0);
        Text("DECOMPRESS_ON_N_ZPLANES__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.decompress_on_n_zplanes);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ALLOW_EXPCLEAR");
        TableSetColumnIndex(1);
        Text("%X", reg.allow_expclear);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("READ_SIZE");
        TableSetColumnIndex(1);
        Text("%X", reg.read_size);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("TILE_SURFACE_ENABLE");
        TableSetColumnIndex(1);
        Text("%X", reg.tile_surface_enable);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("CLEAR_DISALLOWED__VI");
        TableSetColumnIndex(1);
        Text("%X", reg.clear_disallowed);

        TableNextRow();
        TableSetColumnIndex(0);
        Text("ZRANGE_PRECISION");
        TableSetColumnIndex(1);
        Text("%X", reg.zrange_precision);

        if (begin_table) {
            EndTable();
        }
    }
}

void CmdListViewer::OnNop(AmdGpu::PM4Type3Header const* header, u32 const* body) {
    using namespace std::string_view_literals;

#define NOP_PAYLOAD                                                                                \
    P(PUSH_MARKER, 0x68750001)                                                                     \
    P(POP_MARKER, 0x68750002)                                                                      \
    P(SET_MARKER, 0x68750003)                                                                      \
    P(SET_VSHARP, 0x68750004)                                                                      \
    P(SET_TSHARP, 0x68750005)                                                                      \
    P(SET_SSHARP, 0x68750006)                                                                      \
    P(ACB_SUBMIT_MRK, 0x68750013)                                                                  \
    P(SET_USER_DATA, 0x6875000D)                                                                   \
    P(PATCHED_FLIP, 0x68750776)                                                                    \
    P(PREPARE_FLIP, 0x68750777)                                                                    \
    P(PREPARE_FLIP_LABEL, 0x68750778)                                                              \
    P(PREPARE_FLIP_INTERRUPT, 0x68750780)                                                          \
    P(PREPARE_FLIP_INTERRUPT_LABEL, 0x68750781)                                                    \
    P(ALLOC_ALIGN8, 0x68753000)

    auto get_nop_payload_text = [](u32 const nop_payload) {
        switch (nop_payload) {
#define P(name, value)                                                                             \
    case value:                                                                                    \
        return #name##sv;
            NOP_PAYLOAD
#undef P
        default:
            return ""sv;
        }
    };

    Separator();
    BeginGroup();

    auto const* pkt = reinterpret_cast<AmdGpu::PM4CmdNop const*>(header);
    auto const* payload = &body[0];

    // Dump payload
    for (unsigned i = 0; i < pkt->header.count + 1; ++i) {
        Text("%02X: %08X", i, payload[i]);
        if ((payload[i] & 0xffff0000) == 0x68750000) {
            const auto& e = get_nop_payload_text(payload[i]);
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

    // auto const* pkt = reinterpret_cast<AmdGpu::PM4CmdSetBase const*>(header);
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

CmdListViewer::CmdListViewer(DebugStateType::FrameDump* _frame_dump,
                             const std::vector<u32>& cmd_list, uintptr_t _base_addr,
                             std::string _name)
    : frame_dump(_frame_dump), base_addr(_base_addr), name(std::move(_name)) {
    using namespace AmdGpu;

    cmdb_addr = (uintptr_t)cmd_list.data();
    cmdb_size = cmd_list.size() * sizeof(u32);

    cmdb_view_name = fmt::format("[GFX] Command buffer {}###cmdview_hex_{}", this->name, cmdb_addr);
    cmdb_view.Open = false;
    cmdb_view.ReadOnly = true;

    auto const* pm4_hdr = reinterpret_cast<PM4Header const*>(cmdb_addr);

    size_t processed_size = 0;
    size_t prev_offset = 0;
    u32 batch_id = 0;

    std::string marker{};

    if (cmdb_size > 0) {
        events.emplace_back(BatchBegin{.id = 0});
    }

    while (processed_size < cmdb_size) {
        auto* next_pm4_hdr = GetNext(pm4_hdr, 1);
        auto processed_len =
            reinterpret_cast<uintptr_t>(next_pm4_hdr) - reinterpret_cast<uintptr_t>(pm4_hdr);
        processed_size += processed_len;

        if (pm4_hdr->type == PM4Type3Header::TYPE) {

            auto const* pm4_t3 = reinterpret_cast<PM4Type3Header const*>(pm4_hdr);
            auto opcode = pm4_t3->opcode;

            if (opcode == PM4ItOpcode::Nop) {
                auto const* it_body = reinterpret_cast<uint32_t const*>(pm4_hdr + 1);
                switch (it_body[0]) {
                case PM4CmdNop::PayloadType::DebugSetMarker:
                    marker = std::string{(char*)&it_body[1]};
                    break;
                case PM4CmdNop::PayloadType::DebugMarkerPush:
                    events.emplace_back(PushMarker{
                        .name = std::string{(char*)&it_body[1]},
                    });
                    break;
                case PM4CmdNop::PayloadType::DebugMarkerPop:
                    events.emplace_back(PopMarker{});
                    break;
                default:
                    break;
                }
            }

            if (IsDrawCall(opcode)) {
                // All these commands are terminated by NOP at the end, so
                // it is safe to skip it to be even with CP
                // next_pm4_hdr = get_next(next_pm4_hdr, 1);
                // auto constexpr nop_len = 0x10;
                // processed_len += nop_len;
                // processed_size += nop_len;

                events.emplace_back(BatchInfo{
                    .id = batch_id++,
                    .marker = marker,
                    .start_addr = prev_offset,
                    .end_addr = processed_size,
                    .command_addr = processed_size - processed_len,
                    .type = opcode,
                });
                prev_offset = processed_size;
                marker.clear();
                events.emplace_back(BatchBegin{.id = batch_id});
            }
        }

        pm4_hdr = next_pm4_hdr;
    }

    // state batch (last)
    if (processed_size - prev_offset > 0) {
        events.emplace_back(BatchInfo{
            .id = batch_id++,
            .marker = marker,
            .start_addr = prev_offset,
            .end_addr = processed_size,
            .command_addr = 0,
            .type = static_cast<PM4ItOpcode>(0xFF),
        });
    }
    if (!events.empty() && std::holds_alternative<BatchBegin>(events.back())) {
        events.pop_back();
    }
}

void CmdListViewer::Draw(bool only_batches_view, CmdListFilter& filter) {
    const auto& ctx = *GetCurrentContext();

    if (batch_view.open) {
        batch_view.Draw();
    }
    for (auto it = extra_batch_view.begin(); it != extra_batch_view.end();) {
        if (!it->open) {
            it = extra_batch_view.erase(it);
            continue;
        }
        it->Draw();
        ++it;
    }

    if (only_batches_view) {
        return;
    }

    if (cmdb_view.Open) {
        MemoryEditor::Sizes s;
        cmdb_view.CalcSizes(s, cmdb_size, cmdb_addr);
        SetNextWindowSize({s.WindowWidth, s.WindowWidth * 0.6f}, ImGuiCond_FirstUseEver);
        SetNextWindowSizeConstraints({0.0f}, {s.WindowWidth, FLT_MAX});
        if (Begin(cmdb_view_name.c_str(), &cmdb_view.Open,
                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings)) {
            cmdb_view.DrawContents((void*)cmdb_addr, cmdb_size, base_addr);
            if (cmdb_view.ContentsWidthChanged) {
                cmdb_view.CalcSizes(s, cmdb_size, cmdb_addr);
                SetWindowSize({s.WindowWidth, s.WindowWidth * 0.6f});
            }
        }
        End();
    }

    PushID(name.c_str());
    if (BeginChild("cmd_queue", {})) {

        Checkbox("Group batches", &group_batches);
        SameLine();
        Checkbox("Show markers", &show_markers);

        char queue_name[32]{};
        if (vqid < 254) {
            std::snprintf(queue_name, sizeof(queue_name), "%s %d", vqid > 254 ? "GFX" : "ASC",
                          vqid);
        } else {
            std::snprintf(queue_name, sizeof(queue_name), "%s", vqid > 254 ? "GFX" : "ASC");
        }

        Text("queue    : %s", queue_name);
        Text("base addr: %08" PRIXPTR, cmdb_addr);
        SameLine();
        if (SmallButton("Memory >")) {
            cmdb_view.Open ^= true;
        }
        Text("size     : %04zX", cmdb_size);
        Separator();

        {
            int tree_depth = 0;
            int tree_depth_show = 0;

            u32 last_batch_id = ~0u;
            if (!events.empty() && std::holds_alternative<BatchInfo>(events.back())) {
                last_batch_id = std::get<BatchInfo>(events.back()).id;
            }

            u32 batch_id = ~0u;
            u32 current_highlight_batch = ~0u;

            int id = 0;
            PushID(0);
            for (const auto& event : events) {
                PopID();
                PushID(id++);

                if (std::holds_alternative<BatchBegin>(event)) {
                    batch_id = std::get<BatchBegin>(event).id;
                }

                if (show_markers) {
                    if (std::holds_alternative<PushMarker>(event)) {
                        if (tree_depth_show >= tree_depth) {
                            auto& marker = std::get<PushMarker>(event);
                            bool show = TreeNode(&event, "%s", marker.name.c_str());
                            if (show) {
                                tree_depth_show++;
                            }
                        }
                        tree_depth++;
                        continue;
                    }
                    if (std::holds_alternative<PopMarker>(event)) {
                        if (tree_depth_show >= tree_depth) {
                            tree_depth_show--;
                            TreePop();
                        }
                        tree_depth--;
                        continue;
                    }
                    if (tree_depth_show < tree_depth) {
                        continue;
                    }
                }

                if (!std::holds_alternative<BatchInfo>(event)) {
                    continue;
                }

                auto& batch = std::get<BatchInfo>(event);

                // filtering
                {
                    bool remove = false;

                    if (filter.shader_name[0] != '\0') {
                        remove = true;
                        std::string_view shader_name{filter.shader_name};
                        const auto& data = frame_dump->regs.find(batch.command_addr);
                        if (data != frame_dump->regs.end()) {
                            DebugStateType::RegDump& dump = data->second;
                            if (dump.is_compute) {
                                if (dump.cs_data.name.contains(shader_name)) {
                                    remove = false;
                                    break;
                                }
                            } else {
                                for (int i = 0; i < DebugStateType::RegDump::MaxShaderStages; ++i) {
                                    if (dump.regs.stage_enable.IsStageEnabled(i)) {
                                        auto& stage = dump.stages[i];
                                        if (stage.name.contains(shader_name)) {
                                            remove = false;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (remove) {
                        continue;
                    }
                }

                auto const* pm4_hdr =
                    reinterpret_cast<PM4Header const*>(cmdb_addr + batch.start_addr);

                bool ignore_header = false;
                char batch_hdr[128];
                if (batch.type == static_cast<AmdGpu::PM4ItOpcode>(0xFF)) {
                    ignore_header = true;
                } else if (!batch.marker.empty()) {
                    snprintf(batch_hdr, sizeof(batch_hdr), "%08" PRIXPTR ": batch-%03d %s | %s",
                             cmdb_addr + batch.start_addr, batch.id,
                             Gcn::GetOpCodeName(static_cast<u32>(batch.type)),
                             batch.marker.c_str());
                } else {
                    snprintf(batch_hdr, sizeof(batch_hdr), "%08" PRIXPTR ": batch-%03d %s",
                             cmdb_addr + batch.start_addr, batch.id,
                             Gcn::GetOpCodeName(static_cast<u32>(batch.type)));
                }

                if (batch.id == batch_bp) { // highlight batch at breakpoint
                    PushStyleColor(ImGuiCol_Header, ImVec4{1.0f, 0.5f, 0.5f, 0.5f});
                }
                if (batch.id == highlight_batch && !group_batches) {
                    PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 0.7f, 0.7f, 1.0f});
                }

                const auto open_batch_view = [&, this] {
                    if (frame_dump->regs.contains(batch.command_addr)) {
                        auto data = frame_dump->regs.at(batch.command_addr);
                        if (GetIO().KeyShift) {
                            auto& pop = extra_batch_view.emplace_back();
                            pop.SetData(data, name, batch_id);
                            pop.open = true;
                        } else {
                            if (batch_view.open &&
                                this->last_selected_batch == static_cast<int>(batch_id)) {
                                batch_view.open = false;
                            } else {
                                this->last_selected_batch = static_cast<int>(batch_id);
                                batch_view.SetData(data, name, batch_id);
                                if (!batch_view.open || !batch_view.moved) {
                                    batch_view.open = true;
                                    const auto pos = GetItemRectMax() + ImVec2{5.0f, 0.0f};
                                    batch_view.SetPos(pos);
                                }
                            }
                        }
                    }
                };

                bool show_batch_content = true;

                if (group_batches && !ignore_header) {
                    show_batch_content =
                        CollapsingHeader(batch_hdr, ImGuiTreeNodeFlags_AllowOverlap);
                    SameLine(GetContentRegionAvail().x - 40.0f);
                    const char* text =
                        last_selected_batch == static_cast<int>(batch_id) && batch_view.open ? "X"
                                                                                             : "->";
                    if (Button(text, {40.0f, 0.0f})) {
                        open_batch_view();
                    }
                }

                if (show_batch_content) {
                    size_t processed_size = 0;
                    auto bb = ctx.LastItemData.Rect;
                    if (group_batches && !ignore_header) {
                        Indent();
                    }
                    auto const batch_sz = batch.end_addr - batch.start_addr;

                    while (processed_size < batch_sz) {
                        AmdGpu::PM4ItOpcode op{0xFFu};

                        if (pm4_hdr->type == AmdGpu::PM4Type3Header::TYPE) {
                            auto const* pm4_t3 =
                                reinterpret_cast<AmdGpu::PM4Type3Header const*>(pm4_hdr);
                            op = pm4_t3->opcode;

                            char header_name[128];
                            snprintf(header_name, sizeof(header_name), "%08" PRIXPTR ": %s",
                                     cmdb_addr + batch.start_addr + processed_size,
                                     Gcn::GetOpCodeName(static_cast<u32>(op)));

                            bool open_pm4 = TreeNode(header_name);
                            if (!group_batches) {
                                if (IsDrawCall(op)) {
                                    SameLine(GetContentRegionAvail().x - 40.0f);
                                    const char* text =
                                        last_selected_batch == static_cast<int>(batch_id) &&
                                                batch_view.open
                                            ? "X"
                                            : "->";
                                    if (Button(text, {40.0f, 0.0f})) {
                                        open_batch_view();
                                    }
                                }
                                if (IsItemHovered() && ctx.IO.KeyShift) {
                                    if (BeginTooltip()) {
                                        Text("Batch %d", batch_id);
                                        EndTooltip();
                                    }
                                }
                            }
                            if (open_pm4) {
                                if (IsItemToggledOpen()) {
                                    // Editor
                                    cmdb_view.GotoAddrAndHighlight(
                                        reinterpret_cast<size_t>(pm4_hdr) - cmdb_addr,
                                        reinterpret_cast<size_t>(pm4_hdr) - cmdb_addr +
                                            (pm4_hdr->count + 2) * 4);
                                }

                                if (BeginTable("split", 1)) {
                                    TableNextColumn();
                                    Text("size: %d", pm4_hdr->count + 1);

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

                    if (group_batches && !ignore_header) {
                        Unindent();
                    };
                    bb = {{0.0f, bb.Max.y}, ctx.LastItemData.Rect.Max};
                    if (bb.Contains(ctx.IO.MousePos)) {
                        current_highlight_batch = batch.id;
                    }
                }

                if (batch.id == highlight_batch && !group_batches) {
                    PopStyleColor();
                }

                if (batch.id == batch_bp) {
                    PopStyleColor();
                }

                if (batch.id == last_batch_id) {
                    Separator();
                }
            }
            PopID();

            highlight_batch = current_highlight_batch;
        }
    }
    EndChild();
    PopID();
}

} // namespace Core::Devtools::Widget
