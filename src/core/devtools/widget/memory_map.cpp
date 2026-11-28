//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <cinttypes>
#include <mutex>
#include <shared_mutex>
#include <imgui.h>
#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_flags.hpp>

#include "core/debug_state.h"
#include "core/memory/kernel.h"
#include "memory_map.h"

using namespace ImGui;

namespace Core::Devtools::Widget {

static const char* GetEntryType(const VmMapEntry& entry) {
    if (!entry.object) {
        return "Reserved";
    }
    if (entry.IsBlockpool()) {
        return "Pooled";
    }
    if (entry.IsDmem()) {
        return "Direct";
    }
    switch (entry.object->type) {
    case VmObjectType::Default:
        return "Flexible";
    case VmObjectType::Self:
        return "Code";
    case VmObjectType::Vnode:
        return "File";
    default:
        return magic_enum::enum_name(entry.object->type).data();
    }
}

void MemoryMapViewer::DrawVirtualMemory() {
    auto& vm_map = Memory::Instance()->GetVmMap();
    std::shared_lock lk{vm_map.lock};
    const auto& tree = vm_map.GetTree();
    for (auto it = tree.begin(); it != tree.end(); ++it) {
        const auto& entry = *it;
        TableNextColumn();
        Text("%" PRIXPTR, entry.start);
        TableNextColumn();
        Text("%" PRIX64, entry.Size());
        TableNextColumn();
        Text("%s", GetEntryType(entry));
        TableNextColumn();
        Text("%s", magic_enum::enum_flags_name(entry.protection).c_str());
        TableNextColumn();
        if (True(entry.protection & MemoryProt::CpuExec)) {
            Text("X");
        }
        TableNextColumn();
        Text("%s", entry.name);
    }
}

void MemoryMapViewer::DrawDirectMemory() {
    Memory::Instance()->GetDmemManager().ForEachEntry([](const DmemEntry& entry) {
        TableNextColumn();
        Text("%" PRIXPTR, entry.start);
        TableNextColumn();
        Text("%" PRIX64, entry.Size());
        TableNextColumn();
        Text("%s", magic_enum::enum_name(entry.mtype).data());
    });
}

void MemoryMapViewer::Draw() {
    SetNextWindowSize({600.0f, 500.0f}, ImGuiCond_FirstUseEver);
    if (!Begin("Memory map", &open)) {
        End();
        return;
    }

    {
        bool next_showing_vma = showing_vma;
        if (showing_vma) {
            PushStyleColor(ImGuiCol_Button, ImVec4{1.0f, 0.7f, 0.7f, 1.0f});
        }
        if (Button("VMem")) {
            next_showing_vma = true;
        }
        if (showing_vma) {
            PopStyleColor();
        }
        SameLine();
        if (!showing_vma) {
            PushStyleColor(ImGuiCol_Button, ImVec4{1.0f, 0.7f, 0.7f, 1.0f});
        }
        if (Button("DMem")) {
            next_showing_vma = false;
        }
        if (!showing_vma) {
            PopStyleColor();
        }
        showing_vma = next_showing_vma;
    }

    if (BeginTable("memory_view_table", showing_vma ? 6 : 3,
                   ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_RowBg |
                       ImGuiTableFlags_SizingFixedFit)) {
        if (showing_vma) {
            TableSetupColumn("Address");
            TableSetupColumn("Size");
            TableSetupColumn("Type");
            TableSetupColumn("Prot");
            TableSetupColumn("Is Exec");
            TableSetupColumn("Name");
            TableHeadersRow();
            DrawVirtualMemory();
        } else {
            TableSetupColumn("Address");
            TableSetupColumn("Size");
            TableSetupColumn("Type");
            TableHeadersRow();
            DrawDirectMemory();
        }
        EndTable();
    }

    End();
}

} // namespace Core::Devtools::Widget
