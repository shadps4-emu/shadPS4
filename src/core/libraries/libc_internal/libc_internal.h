// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::LibcInternal {

// I won't manage definitons of 3000+ functions, and they don't need to be accessed externally,
// so everything is just in the .cpp file

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::LibcInternal