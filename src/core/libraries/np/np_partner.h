// SPDX-FileCopyrightText: Copyright 2025 shadBloodborne Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpPartner {

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpPartner