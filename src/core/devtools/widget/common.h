//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <variant>

#include <magic_enum.hpp>

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

template <typename T, typename V = u32>
void DrawEnumRow(const char* text, T value) {
    DrawRow(text, "%X (%s)", V(value), magic_enum::enum_name(value).data());
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

static bool IsDrawCall(AmdGpu::PM4ItOpcode opcode) {
    using AmdGpu::PM4ItOpcode;
    switch (opcode) {
    case PM4ItOpcode::DrawIndex2:
    case PM4ItOpcode::DrawIndexOffset2:
    case PM4ItOpcode::DrawIndexAuto:
    case PM4ItOpcode::DrawIndirect:
    case PM4ItOpcode::DrawIndexIndirect:
    case PM4ItOpcode::DispatchDirect:
    case PM4ItOpcode::DispatchIndirect:
        return true;
    default:
        return false;
    }
}

} // namespace Core::Devtools::Widget