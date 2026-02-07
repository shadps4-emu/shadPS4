//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <cinttypes>
#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "core/debug_state.h"
#include "core/memory.h"
#include "memory_map.h"

using namespace ImGui;

namespace Core::Devtools::Widget {

bool MemoryMapViewer::Iterator::DrawLine() {
    if (is_vma) {
        if (vma.it == vma.end) {
            return false;
        }
        auto m = vma.it->second;
        if (m.type == VMAType::Free) {
            ++vma.it;
            return DrawLine();
        }
        TableNextColumn();
        Text("%" PRIXPTR, m.base);
        TableNextColumn();
        Text("%" PRIX64, m.size);
        TableNextColumn();
        Text("%s", magic_enum::enum_name(m.type).data());
        TableNextColumn();
        Text("%s", magic_enum::enum_name(m.prot).data());
        TableNextColumn();
        if (True(m.prot & MemoryProt::CpuExec)) {
            Text("X");
        }
        TableNextColumn();
        Text("%s", m.name.c_str());
        ++vma.it;
        return true;
    }
    if (dmem.it == dmem.end) {
        return false;
    }
    auto m = dmem.it->second;
    if (m.dma_type == PhysicalMemoryType::Free) {
        ++dmem.it;
        return DrawLine();
    }
    TableNextColumn();
    Text("%" PRIXPTR, m.base);
    TableNextColumn();
    Text("%" PRIX64, m.size);
    TableNextColumn();
    auto type = static_cast<::Libraries::Kernel::MemoryTypes>(m.memory_type);
    Text("%s", magic_enum::enum_name(type).data());
    TableNextColumn();
    Text("%d",
         m.dma_type == PhysicalMemoryType::Pooled || m.dma_type == PhysicalMemoryType::Committed);
    ++dmem.it;
    return true;
}

void MemoryMapViewer::Draw() {
    SetNextWindowSize({600.0f, 500.0f}, ImGuiCond_FirstUseEver);
    if (!Begin("Memory map", &open)) {
        End();
        return;
    }

    auto mem = Memory::Instance();
    std::scoped_lock lck{mem->mutex};

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

    Iterator it{};
    if (showing_vma) {
        it.is_vma = true;
        it.vma.it = mem->vma_map.begin();
        it.vma.end = mem->vma_map.end();
    } else {
        it.is_vma = false;
        it.dmem.it = mem->dmem_map.begin();
        it.dmem.end = mem->dmem_map.end();
    }

    if (BeginTable("memory_view_table", showing_vma ? 6 : 4,
                   ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_RowBg |
                       ImGuiTableFlags_SizingFixedFit)) {
        if (showing_vma) {
            TableSetupColumn("Address");
            TableSetupColumn("Size");
            TableSetupColumn("Type");
            TableSetupColumn("Prot");
            TableSetupColumn("Is Exec");
            TableSetupColumn("Name");
        } else {
            TableSetupColumn("Address");
            TableSetupColumn("Size");
            TableSetupColumn("Type");
            TableSetupColumn("Pooled");
        }
        TableHeadersRow();

        while (it.DrawLine())
            ;
        EndTable();
    }

    End();
}

} // namespace Core::Devtools::Widget