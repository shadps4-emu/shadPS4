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

void RegPopup::DrawColorBuffer(const AmdGpu::ColorBuffer& buffer) {
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
            ParseColor0Info(buffer.info.raw, false);
            TreePop();
        }

        TableNextRow();
        TableNextColumn();
        if (TreeNode("Color0Attrib")) {
            TableNextRow();
            TableNextColumn();
            ParseColor0Attrib(buffer.attrib.raw, false);
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

void RegPopup::DrawDepthBuffer(const AmdGpu::DepthBuffer& buffer,
                               const AmdGpu::DepthControl control) {
    SeparatorText("Depth buffer");

    if (BeginTable("DEPTH_BUFFER", 2, ImGuiTableFlags_Borders)) {
        TableNextRow();

        // clang-format off
        DrawValueRowList(
            "Z_INFO.FORMAT",                  buffer.z_info.format,
            "Z_INFO.NUM_SAMPLES",             buffer.z_info.num_samples,
            "Z_INFO.TILE_SPLIT",              buffer.z_info.tile_split,
            "Z_INFO.TILE_MODE_INDEX",         buffer.z_info.tile_mode_index,
            "Z_INFO.DECOMPRESS_ON_N_ZPLANES", buffer.z_info.decompress_on_n_zplanes,
            "Z_INFO.ALLOW_EXPCLEAR",          buffer.z_info.allow_expclear,
            "Z_INFO.READ_SIZE",               buffer.z_info.read_size,
            "Z_INFO.TILE_SURFACE_ENABLE",     buffer.z_info.tile_surface_enable,
            "Z_INFO.CLEAR_DISALLOWED",        buffer.z_info.clear_disallowed,
            "Z_INFO.ZRANGE_PRECISION",        buffer.z_info.zrange_precision,
            "STENCIL_INFO.FORMAT",            buffer.stencil_info.format,
            "Z_READ_BASE",                    buffer.z_read_base,
            "STENCIL_READ_BASE",              buffer.stencil_read_base,
            "Z_WRITE_BASE",                   buffer.z_write_base,
            "STENCIL_WRITE_BASE",             buffer.stencil_write_base,
            "DEPTH_SIZE.PITCH_TILE_MAX",      buffer.depth_size.pitch_tile_max,
            "DEPTH_SIZE.HEIGHT_TILE_MAX",     buffer.depth_size.height_tile_max,
            "DEPTH_SLICE.TILE_MAX",           buffer.depth_slice.tile_max,
            "Pitch()",                        buffer.Pitch(),
            "Height()",                       buffer.Height(),
            "DepthAddress()",                 buffer.DepthAddress(),
            "StencilAddress()",               buffer.StencilAddress(),
            "NumSamples()",                   buffer.NumSamples(),
            "NumBits()",                      buffer.NumBits(),
            "GetDepthSliceSize()",            buffer.GetDepthSliceSize()
        );
        // clang-format on

        EndTable();
    }
    SeparatorText("Depth control");
    if (BeginTable("DEPTH_CONTROL", 2, ImGuiTableFlags_Borders)) {
        TableNextRow();

        // clang-format off
        DrawValueRowList(
            "STENCIL_ENABLE",                     control.stencil_enable,
            "DEPTH_ENABLE",                       control.depth_enable,
            "DEPTH_WRITE_ENABLE",                 control.depth_write_enable,
            "DEPTH_BOUNDS_ENABLE",                control.depth_bounds_enable,
            "DEPTH_FUNC",                         control.depth_func,
            "BACKFACE_ENABLE",                    control.backface_enable,
            "STENCIL_FUNC",                       control.stencil_ref_func,
            "STENCIL_FUNC_BF",                    control.stencil_bf_func,
            "ENABLE_COLOR_WRITES_ON_DEPTH_FAIL",  control.enable_color_writes_on_depth_fail,
            "DISABLE_COLOR_WRITES_ON_DEPTH_PASS", control.disable_color_writes_on_depth_pass
        );
        // clang-format on

        EndTable();
    }
}

RegPopup::RegPopup() {
    static int unique_id = 0;
    id = unique_id++;
}

void RegPopup::SetData(const std::string& base_title, AmdGpu::ColorBuffer color_buffer, u32 cb_id) {
    this->type = DataType::Color;
    this->color = color_buffer;
    this->title = fmt::format("{}/CB #{}", base_title, cb_id);
}

void RegPopup::SetData(const std::string& base_title, AmdGpu::DepthBuffer depth_buffer,
                       AmdGpu::DepthControl depth_control) {
    this->type = DataType::Depth;
    this->depth.buffer = depth_buffer;
    this->depth.control = depth_control;
    this->title = fmt::format("{}/Depth", base_title);
}

void RegPopup::SetPos(ImVec2 pos, bool auto_resize) {
    char name[128];
    snprintf(name, sizeof(name), "%s###reg_popup_%d", title.c_str(), id);
    Begin(name, &open, flags);
    SetWindowPos(pos);
    if (auto_resize) {
        if (type == DataType::Color) {
            SetWindowSize({365.0f, 520.0f});
            KeepWindowInside();
        } else if (type == DataType::Depth) {
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

        if (type == DataType::Color) {
            DrawColorBuffer(color);
        } else if (type == DataType::Depth) {
            DrawDepthBuffer(depth.buffer, depth.control);
        }
    }
    End();
}
} // namespace Core::Devtools::Widget
