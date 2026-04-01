// SPDX-FileCopyrightText: Copyright 2024 shadBloodborne Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Posix {
void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Posix
