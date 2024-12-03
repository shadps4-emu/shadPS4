//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_list.h"

#include <imgui.h>

#include "common.h"
#include "common/config.h"
#include "core/debug_state.h"
#include "core/devtools/options.h"
#include "imgui/imgui_std.h"

using namespace ImGui;

namespace Core::Devtools::Widget {

void ShaderList::DrawShader(DebugStateType::ShaderDump& value) {
    if (!loaded_data) {
        loaded_data = true;
        if (value.cache_raw_disasm.empty()) {
            value.cache_raw_disasm = RunDisassembler(Options.disassembler_cli_isa, value.raw_code);
        }
        isa_editor.SetText(value.cache_raw_disasm);

        if (value.cache_spv_disasm.empty()) {
            value.cache_spv_disasm = RunDisassembler(Options.disassembler_cli_spv, value.spv);
        }
        spv_editor.SetText(value.cache_spv_disasm);
    }

    if (SmallButton("<-")) {
        selected_shader = -1;
    }
    SameLine();
    Text("%s", value.name.c_str());
    SameLine(0.0f, 7.0f);
    if (BeginCombo("Shader type", showing_isa ? "ISA" : "SPIRV", ImGuiComboFlags_WidthFitPreview)) {
        if (Selectable("SPIRV")) {
            showing_isa = false;
        }
        if (Selectable("ISA")) {
            showing_isa = true;
        }
        EndCombo();
    }

    if (showing_isa) {
        isa_editor.Render("ISA", GetContentRegionAvail());
    } else {
        spv_editor.Render("SPIRV", GetContentRegionAvail());
    }
}

ShaderList::ShaderList() {
    isa_editor.SetPalette(TextEditor::GetDarkPalette());
    isa_editor.SetReadOnly(true);
    spv_editor.SetPalette(TextEditor::GetDarkPalette());
    spv_editor.SetReadOnly(true);
    spv_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
}

void ShaderList::Draw() {
    SetNextWindowSize({500.0f, 600.0f}, ImGuiCond_FirstUseEver);
    if (!Begin("Shader list", &open)) {
        End();
        return;
    }

    if (!Config::collectShadersForDebug()) {
        DrawCenteredText("Enable 'CollectShader' in config to see shaders");
        End();
        return;
    }

    if (selected_shader >= 0) {
        DrawShader(DebugState.shader_dump_list[selected_shader]);
        End();
        return;
    }

    auto width = GetContentRegionAvail().x;
    int i = 0;
    for (const auto& shader : DebugState.shader_dump_list) {
        if (ButtonEx(shader.name.c_str(), {width, 20.0f}, ImGuiButtonFlags_NoHoveredOnFocus)) {
            selected_shader = i;
            loaded_data = false;
        }
        i++;
    }

    End();
}

} // namespace Core::Devtools::Widget