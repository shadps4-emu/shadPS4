//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "reg_popup.h"

#include <cstdio>
#include <imgui.h>
#include <magic_enum.hpp>

#include "cmd_list.h"
#include "common.h"

using namespace ImGui;
using magic_enum::enum_name;

namespace Core::Devtools::Widget {

void RegPopup::DrawColorBuffer(const AmdGpu::Liverpool::ColorBuffer& buffer) {
    if (BeginTable("COLOR_BUFFER", 2, ImGuiTableFlags_Borders)) {
        TableNextRow();

        // clang-format off

        DrawMultipleRow(
            "BASE_ADDR",            "%X", buffer.base_address,
            "PITCH.TILE_MAX",       "%X", buffer.pitch.tile_max,
            "PITCH.FMASK_TILE_MAX", "%X", buffer.pitch.fmask_tile_max,
            "SLICE.TILE_MAX",       "%X", buffer.slice.tile_max,
            "VIEW.SLICE_START",     "%X", buffer.view.slice_start,
            "VIEW.SLICE_MAX",       "%X", buffer.view.slice_max
        );

        TableNextRow();
        TableNextColumn();
        if (TreeNode("Color0Info")) {
            TableNextRow();
            TableNextColumn();
            ParseColor0Info(buffer.info.u32all, false);
            TreePop();
        }

        TableNextRow();
        TableNextColumn();
        if (TreeNode("Color0Attrib")) {
            TableNextRow();
            TableNextColumn();
            ParseColor0Attrib(buffer.attrib.u32all, false);
            TreePop();
        }

        TableNextRow();
        DrawMultipleRow(
            "CMASK_BASE_EXT",       "%X", buffer.cmask_base_address,
            "FMASK_BASE_EXT",       "%X", buffer.fmask_base_address,
            "FMASK_SLICE.TILE_MAX", "%X", buffer.fmask_slice.tile_max,
            "CLEAR_WORD0",          "%X", buffer.clear_word0,
            "CLEAR_WORD1",          "%X", buffer.clear_word1
        );

        DrawMultipleRow(
            "Pitch()",             "%X", buffer.Pitch(),
            "Height()",            "%X", buffer.Height(),
            "Address()",           "%X", buffer.Address(),
            "CmaskAddress",        "%X", buffer.CmaskAddress(),
            "FmaskAddress",        "%X", buffer.FmaskAddress(),
            "NumSamples()",        "%X", buffer.NumSamples(),
            "NumSlices()",         "%X", buffer.NumSlices(),
            "GetColorSliceSize()", "%X", buffer.GetColorSliceSize()
        );

        auto tiling_mode = buffer.GetTilingMode();
        auto num_format = buffer.NumFormat();
        DrawEnumRow("GetTilingMode()", tiling_mode);
        DrawRow("IsTiled()", "%X",     buffer.IsTiled());
        DrawEnumRow("NumFormat()",     num_format);

        // clang-format on

        EndTable();
    }
}

void RegPopup::DrawDepthBuffer(const DepthBuffer& depth_data) {
    const auto& [depth_buffer, depth_control] = depth_data;

    SeparatorText("Depth buffer");

    if (BeginTable("DEPTH_BUFFER", 2, ImGuiTableFlags_Borders)) {
        TableNextRow();

        // clang-format off
        DrawEnumRow("Z_INFO.FORMAT", depth_buffer.z_info.format.Value());
        DrawMultipleRow(
            "Z_INFO.NUM_SAMPLES",             "%X", depth_buffer.z_info.num_samples,
            "Z_INFO.TILE_SPLIT",              "%X", depth_buffer.z_info.tile_split,
            "Z_INFO.TILE_MODE_INDEX",         "%X", depth_buffer.z_info.tile_mode_index,
            "Z_INFO.DECOMPRESS_ON_N_ZPLANES", "%X", depth_buffer.z_info.decompress_on_n_zplanes,
            "Z_INFO.ALLOW_EXPCLEAR",          "%X", depth_buffer.z_info.allow_expclear,
            "Z_INFO.READ_SIZE",               "%X", depth_buffer.z_info.read_size,
            "Z_INFO.TILE_SURFACE_EN",         "%X", depth_buffer.z_info.tile_surface_en,
            "Z_INFO.CLEAR_DISALLOWED",        "%X", depth_buffer.z_info.clear_disallowed,
            "Z_INFO.ZRANGE_PRECISION",        "%X", depth_buffer.z_info.zrange_precision
        );

        DrawEnumRow("STENCIL_INFO.FORMAT", depth_buffer.stencil_info.format.Value());

        DrawMultipleRow(
            "Z_READ_BASE",                "%X", depth_buffer.z_read_base,
            "STENCIL_READ_BASE",          "%X", depth_buffer.stencil_read_base,
            "Z_WRITE_BASE",               "%X", depth_buffer.z_write_base,
            "STENCIL_WRITE_BASE",         "%X", depth_buffer.stencil_write_base,
            "DEPTH_SIZE.PITCH_TILE_MAX",  "%X", depth_buffer.depth_size.pitch_tile_max,
            "DEPTH_SIZE.HEIGHT_TILE_MAX", "%X", depth_buffer.depth_size.height_tile_max,
            "DEPTH_SLICE.TILE_MAX",       "%X", depth_buffer.depth_slice.tile_max,
            "Pitch()",                    "%X", depth_buffer.Pitch(),
            "Height()",                   "%X", depth_buffer.Height(),
            "Address()",                  "%X", depth_buffer.Address(),
            "NumSamples()",               "%X", depth_buffer.NumSamples(),
            "NumBits()",                  "%X", depth_buffer.NumBits(),
            "GetDepthSliceSize()",        "%X", depth_buffer.GetDepthSliceSize()
        );
        // clang-format on
        EndTable();
    }
    SeparatorText("Depth control");
    if (BeginTable("DEPTH_CONTROL", 2, ImGuiTableFlags_Borders)) {
        TableNextRow();

        // clang-format off
        DrawMultipleRow(
            "STENCIL_ENABLE",      "%X", depth_control.stencil_enable,
            "DEPTH_ENABLE",        "%X", depth_control.depth_enable,
            "DEPTH_WRITE_ENABLE",  "%X", depth_control.depth_write_enable,
            "DEPTH_BOUNDS_ENABLE", "%X", depth_control.depth_bounds_enable
        );
        DrawEnumRow("DEPTH_FUNC", depth_control.depth_func.Value());
        DrawRow("BACKFACE_ENABLE", "%X", depth_control.backface_enable);
        DrawEnumRow("STENCIL_FUNC", depth_control.stencil_ref_func.Value());
        DrawEnumRow("STENCIL_FUNC_BF", depth_control.stencil_bf_func.Value());
        DrawMultipleRow(
            "ENABLE_COLOR_WRITES_ON_DEPTH_FAIL", "%X", depth_control.enable_color_writes_on_depth_fail,
            "DISABLE_COLOR_WRITES_ON_DEPTH_PASS", "%X", depth_control.disable_color_writes_on_depth_pass
        );
        // clang-format on

        EndTable();
    }
}

RegPopup::RegPopup() {
    static int unique_id = 0;
    id = unique_id++;
}

void RegPopup::SetData(AmdGpu::Liverpool::ColorBuffer color_buffer, u32 batch_id, u32 cb_id) {
    this->data = color_buffer;
    this->title = fmt::format("Batch #{} CB #{}", batch_id, cb_id);
}

void RegPopup::SetData(AmdGpu::Liverpool::DepthBuffer depth_buffer,
                       AmdGpu::Liverpool::DepthControl depth_control, u32 batch_id) {
    this->data = std::make_tuple(depth_buffer, depth_control);
    this->title = fmt::format("Batch #{} Depth", batch_id);
}

void RegPopup::Draw() {

    char name[128];
    snprintf(name, sizeof(name), "%s###reg_popup_%d", title.c_str(), id);

    SetNextWindowSize({250.0f, 300.0f}, ImGuiCond_FirstUseEver);
    if (Begin(name, &open, ImGuiWindowFlags_NoSavedSettings)) {
        if (const auto* buffer = std::get_if<AmdGpu::Liverpool::ColorBuffer>(&data)) {
            DrawColorBuffer(*buffer);
        } else if (const auto* depth_data = std::get_if<DepthBuffer>(&data)) {
            DrawDepthBuffer(*depth_data);
        }
    }
    End();
}

} // namespace Core::Devtools::Widget
