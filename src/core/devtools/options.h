//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

struct ImGuiTextBuffer;

namespace Core::Devtools {

struct TOptions {
    std::string disassembly_cli;
};

extern TOptions Options;

void LoadOptionsConfig(const char* line);
void SerializeOptionsConfig(ImGuiTextBuffer* buf);

} // namespace Core::Devtools
