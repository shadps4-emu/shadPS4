// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "libc_internal_io.h"
#include "libc_internal_math.h"
#include "libc_internal_memory.h"
#include "libc_internal_str.h"
#include "libc_internal_threads.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::LibcInternal {

// I won't manage definitons of 3000+ functions, and they don't need to be accessed externally,
// so everything is just in the .cpp file

struct EngineV1 {
    MathEngine m_math_engine;
    StrEngine m_str_engine;
    MemoryEngine m_memory_engine;
    IoEngineV1 m_io_engine;
    ThreadsEngine m_threads_engine;

    EngineV1(Core::Loader::SymbolsResolver* sym);
};

struct EngineV2 {
    IoEngineV2 m_io_engine;

    EngineV2(Core::Loader::SymbolsResolver* sym);
};

using Engine = EngineV2;

} // namespace Libraries::LibcInternal