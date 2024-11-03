// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Videodec {

int PS4_SYSV_ABI sceVideodecCreateDecoder();
int PS4_SYSV_ABI sceVideodecDecode();
int PS4_SYSV_ABI sceVideodecDeleteDecoder();
int PS4_SYSV_ABI sceVideodecFlush();
int PS4_SYSV_ABI sceVideodecMapMemory();
int PS4_SYSV_ABI sceVideodecQueryResourceInfo();
int PS4_SYSV_ABI sceVideodecReset();

void RegisterlibSceVideodec(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Videodec