//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "reg_popup.h"

#include <cstdio>
#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "cmd_list.h"
#include "common.h"
#include "imgui/imgui_std.h"

using namespace ImGui;
using magic_enum::enum_name;

namespace Core::Devtools::Widget {

void RegPopup::DrawColorBuffer(const AmdGpu::Liverpool::ColorBuffer& buffer) {
    if (BeginTable("COLOR_BUFFER", 2, ImGuiTableFlags_Borders)) {
        TableNextRow();

        // clang-format off

        DrawValueRowList(
            "BASE_ADDR",            buffer.base_address,
            "PITCH.TILE_MAX",       buffer.pitch.tile_max,
            "PITCH.FMASK_TILE_MAX", buffer.pitch.fmask_tile_max,
            "SLICE.TILE_MAX",       buffer.slice.tile_max,
            "VIEW.SLICE_START",     buffer.view.slice_start,
            "VIEW.SLICE_MAX",       buffer.view.slice_max
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
        DrawValueRowList(
            "CMASK_BASE_EXT",       buffer.cmask_base_address,
            "FMASK_BASE_EXT",       buffer.fmask_base_address,
            "FMASK_SLICE.TILE_MAX", buffer.fmask_slice.tile_max,
            "CLEAR_WORD0",          buffer.clear_word0,
            "CLEAR_WORD1",          buffer.clear_word1,
            "Pitch()",              buffer.Pitch(),
            "Height()",             buffer.Height(),
            "Address()",            buffer.Address(),
            "CmaskAddress",         buffer.CmaskAddress(),
            "FmaskAddress",         buffer.FmaskAddress(),
            "NumSamples()",         buffer.NumSamples(),
            "NumSlices()",          buffer.NumSlices(),
            "GetColorSliceSize()",  buffer.GetColorSliceSize(),
            "GetTileMode()",        buffer.GetTileMode(),
            "IsTiled()",            buffer.IsTiled(),
            "NumFormat()",          buffer.GetNumberFmt()
        );

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
        DrawValueRowList(
            "Z_INFO.FORMAT",                  depth_buffer.z_info.format,
            "Z_INFO.NUM_SAMPLES",             depth_buffer.z_info.num_samples,
            "Z_INFO.TILE_SPLIT",              depth_buffer.z_info.tile_split,
            "Z_INFO.TILE_MODE_INDEX",         depth_buffer.z_info.tile_mode_index,
            "Z_INFO.DECOMPRESS_ON_N_ZPLANES", depth_buffer.z_info.decompress_on_n_zplanes,
            "Z_INFO.ALLOW_EXPCLEAR",          depth_buffer.z_info.allow_expclear,
            "Z_INFO.READ_SIZE",               depth_buffer.z_info.read_size,
            "Z_INFO.TILE_SURFACE_EN",         depth_buffer.z_info.tile_surface_en,
            "Z_INFO.CLEAR_DISALLOWED",        depth_buffer.z_info.clear_disallowed,
            "Z_INFO.ZRANGE_PRECISION",        depth_buffer.z_info.zrange_precision,
            "STENCIL_INFO.FORMAT",            depth_buffer.stencil_info.format,
            "Z_READ_BASE",                    depth_buffer.z_read_base,
            "STENCIL_READ_BASE",              depth_buffer.stencil_read_base,
            "Z_WRITE_BASE",                   depth_buffer.z_write_base,
            "STENCIL_WRITE_BASE",             depth_buffer.stencil_write_base,
            "DEPTH_SIZE.PITCH_TILE_MAX",      depth_buffer.depth_size.pitch_tile_max,
            "DEPTH_SIZE.HEIGHT_TILE_MAX",     depth_buffer.depth_size.height_tile_max,
            "DEPTH_SLICE.TILE_MAX",           depth_buffer.depth_slice.tile_max,
            "Pitch()",                        depth_buffer.Pitch(),
            "Height()",                       depth_buffer.Height(),
            "DepthAddress()",                 depth_buffer.DepthAddress(),
            "StencilAddress()",               depth_buffer.StencilAddress(),
            "NumSamples()",                   depth_buffer.NumSamples(),
            "NumBits()",                      depth_buffer.NumBits(),
            "GetDepthSliceSize()",            depth_buffer.GetDepthSliceSize()
        );
        // clang-format on

        EndTable();
    }
    SeparatorText("Depth control");
    if (BeginTable("DEPTH_CONTROL", 2, ImGuiTableFlags_Borders)) {
        TableNextRow();

        // clang-format off
        DrawValueRowList(
            "STENCIL_ENABLE",                     depth_control.stencil_enable,
            "DEPTH_ENABLE",                       depth_control.depth_enable,
            "DEPTH_WRITE_ENABLE",                 depth_control.depth_write_enable,
            "DEPTH_BOUNDS_ENABLE",                depth_control.depth_bounds_enable,
            "DEPTH_FUNC",                         depth_control.depth_func,
            "BACKFACE_ENABLE",                    depth_control.backface_enable,
            "STENCIL_FUNC",                       depth_control.stencil_ref_func,
            "STENCIL_FUNC_BF",                    depth_control.stencil_bf_func,
            "ENABLE_COLOR_WRITES_ON_DEPTH_FAIL",  depth_control.enable_color_writes_on_depth_fail,
            "DISABLE_COLOR_WRITES_ON_DEPTH_PASS", depth_control.disable_color_writes_on_depth_pass
        );
        // clang-format on

        EndTable();
    }
}

RegPopup::RegPopup() {
    static int unique_id = 0;
    id = unique_id++;
}

void RegPopup::SetData(const std::string& base_title, AmdGpu::Liverpool::ColorBuffer color_buffer,
                       u32 cb_id) {
    this->data = color_buffer;
    this->title = fmt::format("{}/CB #{}", base_title, cb_id);
}

void RegPopup::SetData(const std::string& base_title, AmdGpu::Liverpool::DepthBuffer depth_buffer,
                       AmdGpu::Liverpool::DepthControl depth_control) {
    this->data = std::make_tuple(depth_buffer, depth_control);
    this->title = fmt::format("{}/Depth", base_title);
}

void RegPopup::SetPos(ImVec2 pos, bool auto_resize) {
    char name[128];
    snprintf(name, sizeof(name), "%s###reg_popup_%d", title.c_str(), id);
    Begin(name, &open, flags);
    SetWindowPos(pos);
    if (auto_resize) {
        if (std::holds_alternative<AmdGpu::Liverpool::ColorBuffer>(data)) {
            SetWindowSize({365.0f, 520.0f});
            KeepWindowInside();
        } else if (std::holds_alternative<DepthBuffer>(data)) {
            SetWindowSize({404.0f, 543.0f});
            KeepWindowInside();
        }
    }
    last_pos = GetWindowPos();
    moved = false;
    End();
}

void RegPopup::Draw() {
    char name[128];
    snprintf(name, sizeof(name), "%s###reg_popup_%d", title.c_str(), id);
    if (Begin(name, &open, flags)) {
        if (GetWindowPos() != last_pos) {
            moved = true;
        }

        if (const auto* buffer = std::get_if<AmdGpu::Liverpool::ColorBuffer>(&data)) {
            DrawColorBuffer(*buffer);
        } else if (const auto* depth_data = std::get_if<DepthBuffer>(&data)) {
            DrawDepthBuffer(*depth_data);
        }
    }
    End();
}
} // namespace Core::Devtools::Widget
