//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "options.h"

#include <memory>
#include <imgui.h>

#include "video_core/renderer_vulkan/vk_presenter.h"

extern std::unique_ptr<Vulkan::Presenter> presenter;

namespace Core::Devtools {

TOptions Options;

void LoadOptionsConfig(const char* line) {
    char str[512];
    int i;
    float f;
    if (sscanf(line, "disassembler_cli_isa=%511[^\n]", str) == 1) {
        Options.disassembler_cli_isa = str;
        return;
    }
    if (sscanf(line, "disassembler_cli_spv=%511[^\n]", str) == 1) {
        Options.disassembler_cli_spv = str;
        return;
    }
    if (sscanf(line, "frame_dump_render_on_collapse=%d", &i) == 1) {
        Options.frame_dump_render_on_collapse = i != 0;
        return;
    }
    if (sscanf(line, "fsr_enabled=%d", &i) == 1) {
        presenter->GetFsrSettingsRef().enable = i != 0;
        return;
    }
    if (sscanf(line, "fsr_rcas_enabled=%d", &i) == 1) {
        presenter->GetFsrSettingsRef().use_rcas = i != 0;
        return;
    }
    if (sscanf(line, "fsr_rcas_attenuation=%f", &f) == 1) {
        presenter->GetFsrSettingsRef().rcas_attenuation = f;
    }
}

void SerializeOptionsConfig(ImGuiTextBuffer* buf) {
    buf->appendf("disassembler_cli_isa=%s\n", Options.disassembler_cli_isa.c_str());
    buf->appendf("disassembler_cli_spv=%s\n", Options.disassembler_cli_spv.c_str());
    buf->appendf("frame_dump_render_on_collapse=%d\n", Options.frame_dump_render_on_collapse);
    buf->appendf("fsr_enabled=%d\n", presenter->GetFsrSettingsRef().enable);
    buf->appendf("fsr_rcas_enabled=%d\n", presenter->GetFsrSettingsRef().use_rcas);
    buf->appendf("fsr_rcas_attenuation=%f\n", presenter->GetFsrSettingsRef().rcas_attenuation);
}

} // namespace Core::Devtools
