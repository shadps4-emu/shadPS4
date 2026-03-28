// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Ssl2 {

struct OrbisSslData {
    char* ptr;
    u64 size;
};

struct OrbisSslCaCerts {
    OrbisSslData* certs;
    u64 num;
    void* pool;
};

struct Engine {
    Engine(Core::Loader::SymbolsResolver* sym);
};

} // namespace Libraries::Ssl2