//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>

#include "options.h"

namespace Core::Devtools {

TOptions Options;

void LoadOptionsConfig(const char* line) {
    char str[512];
    if (sscanf(line, "disassembly_cli=%511[^\n]", str) == 1) {
        Options.disassembly_cli = str;
        return;
    }
}

void SerializeOptionsConfig(ImGuiTextBuffer* buf) {
    buf->appendf("disassembly_cli=%s\n", Options.disassembly_cli.c_str());
}

} // namespace Core::Devtools
