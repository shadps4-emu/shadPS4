// SPDX-FileCopyrightText: Copyright 2024 shadBloodborne Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

void RegisterDebug(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel