//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <variant>

#include "common/types.h"
#include "video_core/amdgpu/pm4_opcodes.h"

namespace Core::Devtools::Widget {

/*
 * Generic PM4 header
 */
union PM4Header {
    struct {
        u32 reserved : 16;
        u32 count : 14;
        u32 type : 2; // PM4_TYPE
    };
    u32 u32All;
};

struct PushMarker {
    std::string name{};
};

struct PopMarker {};

struct BatchBegin {
    u32 id;
};

struct BatchInfo {
    u32 id;
    std::string marker{};
    size_t start_addr;
    size_t end_addr;
    size_t command_addr;
    AmdGpu::PM4ItOpcode type;
    bool bypass{false};
};

using GPUEvent = std::variant<PushMarker, PopMarker, BatchBegin, BatchInfo>;

template <typename... Args>
void DrawRow(const char* text, const char* fmt, Args... args) {
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(text);
    ImGui::TableNextColumn();
    char buf[128];
    snprintf(buf, sizeof(buf), fmt, args...);
    ImGui::TextUnformatted(buf);
}

template <typename V, typename... Extra>
void DrawMultipleRow(const char* text, const char* fmt, V arg, Extra&&... extra_args) {
    DrawRow(text, fmt, arg);
    if constexpr (sizeof...(extra_args) > 0) {
        DrawMultipleRow(std::forward<Extra>(extra_args)...);
    }
}

template <typename... Args>
static void DoTooltip(const char* str_id, Args&&... args) {
    if (ImGui::BeginTooltip()) {
        if (ImGui::BeginTable(str_id, 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            DrawMultipleRow(std::forward<Args>(args)...);
            ImGui::EndTable();
        }
        ImGui::EndTooltip();
    }
}

} // namespace Core::Devtools::Widget