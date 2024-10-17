//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <type_traits>
#include <variant>

#include <magic_enum.hpp>

#include "common/bit_field.h"
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

template <typename T>
void DrawValueRow(const char* text, T value) {
    if constexpr (std::is_enum_v<T>) {
        return DrawRow(text, "%X (%s)", value, magic_enum::enum_name(value).data());
    } else if constexpr (std::is_integral_v<T>) {
        return DrawRow(text, "%X", value);
    } else if constexpr (std::is_base_of_v<BitField<T::position, T::bits, typename T::Type>, T>) {
        return DrawValueRow(text, value.Value());
    } else {
        static_assert(false, "Unsupported type");
    }
}

template <typename V, typename... Extra>
void DrawValueRowList(const char* text, V arg, Extra&&... extra_args) {
    DrawValueRow(text, arg);
    if constexpr (sizeof...(extra_args) > 0) {
        DrawValueRowList(std::forward<Extra>(extra_args)...);
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