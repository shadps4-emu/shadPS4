// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibC {

void libcSymbolsRegister(Loader::SymbolsResolver* sym);

} // namespace Core::Libraries::LibC
