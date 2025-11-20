//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

// Credits to https://github.com/psucien/tlg-emu-tools/

#pragma once

#include <vector>
#include <imgui.h>

#include "common.h"
#include "common/types.h"
#include "core/devtools/widget/imgui_memory_editor.h"
#include "core/devtools/widget/reg_view.h"

namespace AmdGpu {
union PM4Type3Header;
enum class PM4ItOpcode : u32;
} // namespace AmdGpu

namespace Core::Devtools::Widget {

class FrameDumpViewer;

void ParsePolygonControl(u32 value, bool begin_table = true);
void ParseAaConfig(u32 value, bool begin_table = true);
void ParseViewportControl(u32 value, bool begin_table = true);
void ParseColorControl(u32 value, bool begin_table = true);
void ParseColor0Info(u32 value, bool begin_table = true);
void ParseColor0Attrib(u32 value, bool begin_table = true);
void ParseBlendControl(u32 value, bool begin_table = true);
void ParseDepthRenderControl(u32 value, bool begin_table = true);
void ParseDepthControl(u32 value, bool begin_table = true);
void ParseEqaa(u32 value, bool begin_table = true);
void ParseZInfo(u32 value, bool begin_table = true);

struct CmdListFilter {
    char shader_name[128]{};
};

class CmdListViewer {

    DebugStateType::FrameDump* frame_dump;

    uintptr_t base_addr;
    std::string name;
    std::vector<GPUEvent> events{};
    uintptr_t cmdb_addr;
    size_t cmdb_size;

    std::string cmdb_view_name;
    MemoryEditor cmdb_view;

    int batch_bp{-1};
    int vqid{255};
    u32 highlight_batch{~0u};

    RegView batch_view;
    int last_selected_batch{-1};

    std::vector<RegView> extra_batch_view;

    static void OnNop(AmdGpu::PM4Type3Header const* header, u32 const* body);
    static void OnSetBase(AmdGpu::PM4Type3Header const* header, u32 const* body);
    static void OnSetContextReg(AmdGpu::PM4Type3Header const* header, u32 const* body);
    static void OnSetShReg(AmdGpu::PM4Type3Header const* header, u32 const* body);
    static void OnDispatch(AmdGpu::PM4Type3Header const* header, u32 const* body);

public:
    static void LoadConfig(const char* line);
    static void SerializeConfig(ImGuiTextBuffer* buf);

    explicit CmdListViewer(DebugStateType::FrameDump* frame_dump, const std::vector<u32>& cmd_list,
                           uintptr_t base_addr = 0, std::string name = "");

    void Draw(bool only_batches_view, CmdListFilter& filter);
};

} // namespace Core::Devtools::Widget
