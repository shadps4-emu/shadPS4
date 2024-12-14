//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

struct ImGuiTextBuffer;

namespace Core::Devtools {

struct TOptions {
    std::string disassembler_cli_isa{"clrxdisasm --raw {src}"};
    std::string disassembler_cli_spv{"spirv-cross -V {src}"};
    bool frame_dump_render_on_collapse{false};
};

extern TOptions Options;

void LoadOptionsConfig(const char* line);
void SerializeOptionsConfig(ImGuiTextBuffer* buf);

} // namespace Core::Devtools
