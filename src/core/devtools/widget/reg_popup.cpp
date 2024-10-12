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
        DrawRow("GetTilingMode()", "%X (%s)", tiling_mode, enum_name(tiling_mode).data());
        DrawRow("IsTiled()",       "%X",      buffer.IsTiled());
        DrawRow("NumFormat()",     "%X (%s)", num_format, enum_name(num_format).data());

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

void RegPopup::Draw() {

    char name[128];
    snprintf(name, sizeof(name), "%s###reg_popup_%d", title.c_str(), id);

    SetNextWindowSize({250.0f, 300.0f}, ImGuiCond_FirstUseEver);
    if (Begin(name, &open, ImGuiWindowFlags_NoSavedSettings)) {
        if (const auto* buffer = std::get_if<AmdGpu::Liverpool::ColorBuffer>(&data)) {
            DrawColorBuffer(*buffer);
        }
    }
    End();
}

} // namespace Core::Devtools::Widget
