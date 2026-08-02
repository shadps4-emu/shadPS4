// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::LibcInternal {

s64 PS4_SYSV_ABI internal_time(s64* tloc);

void RegisterlibSceLibcInternalTime(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::LibcInternal
