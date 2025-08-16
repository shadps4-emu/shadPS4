//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <string>
#include <type_traits>
#include <variant>

#include <magic_enum/magic_enum.hpp>

#include "common/bit_field.h"
#include "common/io_file.h"
#include "common/types.h"
#include "core/debug_state.h"
#include "video_core/amdgpu/pm4_opcodes.h"

#if defined(_WIN32)
#define popen _popen
#define pclose _pclose
#endif

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

inline std::optional<std::string> exec_cli(const char* cli) {
    std::array<char, 64> buffer{};
    std::string output;
    const auto f = popen(cli, "r");
    if (!f) {
        pclose(f);
        return {};
    }
    while (fgets(buffer.data(), buffer.size(), f)) {
        output += buffer.data();
    }
    pclose(f);
    return output;
}

template <typename T>
inline std::string RunDisassembler(const std::string& disassembler_cli, const T& shader_code,
                                   bool* success = nullptr) {
    std::string shader_dis;

    if (disassembler_cli.empty()) {
        shader_dis = "No disassembler set";
        if (success) {
            *success = false;
        }
    } else {
        auto bin_path = std::filesystem::temp_directory_path() / "shadps4_tmp_shader.bin";

        constexpr std::string_view src_arg = "{src}";
        std::string cli = disassembler_cli + " 2>&1";
        const auto pos = cli.find(src_arg);
        if (pos == std::string::npos) {
            shader_dis = "Disassembler CLI does not contain {src} argument";
            if (success) {
                *success = false;
            }
        } else {
            cli.replace(pos, src_arg.size(), "\"" + bin_path.string() + "\"");
            Common::FS::IOFile file(bin_path, Common::FS::FileAccessMode::Create);
            file.Write(shader_code);
            file.Close();

            auto result = exec_cli(cli.c_str());
            if (result) {
                shader_dis = result.value();
                if (success) {
                    *success = true;
                }
            } else {
                if (success) {
                    *success = false;
                }
                shader_dis = "Could not disassemble shader";
            }

            std::filesystem::remove(bin_path);
        }
    }

    return shader_dis;
}

} // namespace Core::Devtools::Widget