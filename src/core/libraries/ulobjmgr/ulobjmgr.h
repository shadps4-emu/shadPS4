// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Ulobjmgr {
void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Ulobjmgr