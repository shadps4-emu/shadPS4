//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>

#include "shader_list.h"

#include <imgui.h>

#include "common.h"
#include "common/config.h"
#include "common/path_util.h"
#include "common/string_util.h"
#include "core/debug_state.h"
#include "core/devtools/options.h"
#include "imgui/imgui_std.h"
#include "sdl_window.h"
#include "video_core/renderer_vulkan/vk_presenter.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"

extern std::unique_ptr<Vulkan::Presenter> presenter;

using namespace ImGui;

namespace Core::Devtools::Widget {

ShaderList::Selection::Selection(int index) : index(index) {
    isa_editor.SetPalette(TextEditor::GetDarkPalette());
    isa_editor.SetReadOnly(true);
    glsl_editor.SetPalette(TextEditor::GetDarkPalette());
    glsl_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
    presenter->GetWindow().RequestKeyboard();
}

ShaderList::Selection::~Selection() {
    presenter->GetWindow().ReleaseKeyboard();
}

void ShaderList::Selection::ReloadShader(DebugStateType::ShaderDump& value) {
    auto& spv = value.is_patched ? value.patch_spv : value.spv;
    if (spv.empty()) {
        return;
    }
    auto& cache = presenter->GetRasterizer().GetPipelineCache();
    if (const auto m = cache.ReplaceShader(value.module, spv); m) {
        value.module = *m;
    }
}

bool ShaderList::Selection::DrawShader(DebugStateType::ShaderDump& value) {
    if (!value.loaded_data) {
        value.loaded_data = true;
        if (value.cache_isa_disasm.empty()) {
            value.cache_isa_disasm = RunDisassembler(Options.disassembler_cli_isa, value.isa);
        }
        if (value.cache_spv_disasm.empty()) {
            value.cache_spv_disasm = RunDisassembler(Options.disassembler_cli_spv, value.spv);
        }
        if (!value.patch_spv.empty() && value.cache_patch_disasm.empty()) {
            value.cache_patch_disasm = RunDisassembler("spirv-dis {src}", value.patch_spv);
        }
        patch_path =
            Common::FS::GetUserPath(Common::FS::PathType::ShaderDir) / "patch" / value.name;
        patch_bin_path = patch_path;
        patch_bin_path += ".spv";
        patch_path += ".glsl";
        if (std::filesystem::exists(patch_path)) {
            std::ifstream file{patch_path};
            value.patch_source =
                std::string{std::istreambuf_iterator{file}, std::istreambuf_iterator<char>{}};
        }

        value.is_patched = !value.patch_spv.empty();
        if (!value.is_patched) { // No patch
            isa_editor.SetText(value.cache_isa_disasm);
            glsl_editor.SetText(value.cache_spv_disasm);
        } else {
            isa_editor.SetText(value.cache_patch_disasm);
            isa_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::SPIRV());
            glsl_editor.SetText(value.patch_source);
            glsl_editor.SetReadOnly(false);
        }
    }

    char name[64];
    snprintf(name, sizeof(name), "Shader %s", value.name.c_str());
    SetNextWindowSize({450.0f, 600.0f}, ImGuiCond_FirstUseEver);
    if (!Begin(name, &open, ImGuiWindowFlags_NoNav)) {
        End();
        return open;
    }

    Text("%s", value.name.c_str());
    SameLine(0.0f, 7.0f);
    if (Checkbox("Enable patch", &value.is_patched)) {
        if (value.is_patched) {
            if (value.patch_source.empty()) {
                value.patch_source = value.cache_spv_disasm;
            }
            isa_editor.SetText(value.cache_patch_disasm);
            isa_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::SPIRV());
            glsl_editor.SetText(value.patch_source);
            glsl_editor.SetReadOnly(false);
            if (!value.patch_spv.empty()) {
                ReloadShader(value);
            }
        } else {
            isa_editor.SetText(value.cache_isa_disasm);
            isa_editor.SetLanguageDefinition(TextEditor::LanguageDefinition());
            glsl_editor.SetText(value.cache_spv_disasm);
            glsl_editor.SetReadOnly(true);
            ReloadShader(value);
        }
    }
    SameLine();
    if (Button("Copy name")) {
        SetClipboardText(value.name.c_str());
    }

    if (value.is_patched) {
        if (BeginCombo("Shader type", showing_bin ? "SPIRV" : "GLSL",
                       ImGuiComboFlags_WidthFitPreview)) {
            if (Selectable("GLSL")) {
                showing_bin = false;
            }
            if (Selectable("SPIRV")) {
                showing_bin = true;
            }
            EndCombo();
        }
    } else {
        if (BeginCombo("Shader type", showing_bin ? "ISA" : "GLSL",
                       ImGuiComboFlags_WidthFitPreview)) {
            if (Selectable("GLSL")) {
                showing_bin = false;
            }
            if (Selectable("ISA")) {
                showing_bin = true;
            }
            EndCombo();
        }
    }

    if (value.is_patched) {
        bool save = false;
        bool compile = false;
        SameLine(0.0f, 3.0f);
        if (Button("Save")) {
            save = true;
        }
        SameLine();
        if (Button("Save & Compile")) {
            save = true;
            compile = true;
        }
        if (save) {
            value.patch_source = glsl_editor.GetText();
            std::ofstream file{patch_path, std::ios::binary | std::ios::trunc};
            file << value.patch_source;
            std::string msg = "Patch saved to ";
            msg += Common::U8stringToString(patch_path.u8string());
            DebugState.ShowDebugMessage(msg);
        }
        if (compile) {
            static std::map<Shader::LogicalStage, std::string> stage_arg = {
                {Shader::LogicalStage::Vertex, "vert"},
                {Shader::LogicalStage::TessellationControl, "tesc"},
                {Shader::LogicalStage::TessellationEval, "tese"},
                {Shader::LogicalStage::Geometry, "geom"},
                {Shader::LogicalStage::Fragment, "frag"},
                {Shader::LogicalStage::Compute, "comp"},
            };
            auto stage = stage_arg.find(value.l_stage);
            if (stage == stage_arg.end()) {
                DebugState.ShowDebugMessage(std::string{"Invalid shader stage"});
            } else {
                std::string cmd =
                    fmt::format("glslc --target-env=vulkan1.3 --target-spv=spv1.6 "
                                "-fshader-stage={} {{src}} -o \"{}\"",
                                stage->second, Common::U8stringToString(patch_bin_path.u8string()));
                bool success = false;
                auto res = RunDisassembler(cmd, value.patch_source, &success);
                if (!res.empty() || !success) {
                    DebugState.ShowDebugMessage("Compilation failed:\n" + res);
                } else {
                    Common::FS::IOFile file{patch_bin_path, Common::FS::FileAccessMode::Read};
                    value.patch_spv.resize(file.GetSize() / sizeof(u32));
                    file.Read(value.patch_spv);
                    value.cache_patch_disasm =
                        RunDisassembler("spirv-dis {src}", value.patch_spv, &success);
                    if (!success) {
                        DebugState.ShowDebugMessage("Decompilation failed (Compile was ok):\n" +
                                                    res);
                    } else {
                        isa_editor.SetText(value.cache_patch_disasm);
                        ReloadShader(value);
                    }
                }
            }
        }
    }

    if (showing_bin) {
        isa_editor.Render(value.is_patched ? "SPIRV" : "ISA", GetContentRegionAvail());
    } else {
        glsl_editor.Render("GLSL", GetContentRegionAvail());
    }

    End();
    return open;
}

void ShaderList::Draw() {
    for (auto it = open_shaders.begin(); it != open_shaders.end();) {
        auto& selection = *it;
        auto& shader = DebugState.shader_dump_list[selection.index];
        if (!selection.DrawShader(shader)) {
            it = open_shaders.erase(it);
        } else {
            ++it;
        }
    }

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

    InputTextEx("##search_shader", "Search by name", search_box, sizeof(search_box), {},
                ImGuiInputTextFlags_None);

    auto width = GetContentRegionAvail().x;
    int i = 0;
    for (const auto& shader : DebugState.shader_dump_list) {
        if (search_box[0] != '\0' && !shader.name.contains(search_box)) {
            i++;
            continue;
        }
        char name[128];
        if (shader.is_patched) {
            snprintf(name, sizeof(name), "%s (PATCH ON)", shader.name.c_str());
        } else if (!shader.patch_spv.empty()) {
            snprintf(name, sizeof(name), "%s (PATCH OFF)", shader.name.c_str());
        } else {
            snprintf(name, sizeof(name), "%s", shader.name.c_str());
        }
        if (ButtonEx(name, {width, 20.0f}, ImGuiButtonFlags_NoHoveredOnFocus)) {
            open_shaders.emplace_back(i);
        }
        i++;
    }

    End();
}

} // namespace Core::Devtools::Widget