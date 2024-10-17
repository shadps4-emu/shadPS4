//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>

#include "options.h"

namespace Core::Devtools {

TOptions Options;

void LoadOptionsConfig(const char* line) {
    char str[512];
    int i;
    if (sscanf(line, "disassembly_cli=%511[^\n]", str) == 1) {
        Options.disassembly_cli = str;
        return;
    }
    if (sscanf(line, "frame_dump_render_on_collapse=%d", &i) == 1) {
        Options.frame_dump_render_on_collapse = i != 0;
        return;
    }
}

void SerializeOptionsConfig(ImGuiTextBuffer* buf) {
    buf->appendf("disassembly_cli=%s\n", Options.disassembly_cli.c_str());
    buf->appendf("frame_dump_render_on_collapse=%d\n", Options.frame_dump_render_on_collapse);
}

} // namespace Core::Devtools
