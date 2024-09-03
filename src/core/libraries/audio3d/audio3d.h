// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Audio3d {

class Audio3d;

void RegisterlibSceAudio3d(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Audio3d