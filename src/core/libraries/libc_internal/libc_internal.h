// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/libc_internal/libc_internal_math.h"
#include "core/libraries/libc_internal/libc_internal_str.h"
#include "core/libraries/libc_internal/libc_internal_memory.h"
#include "core/libraries/libc_internal/libc_internal_io.h"
#include "core/libraries/libc_internal/libc_internal_threads.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::LibcInternal {

// I won't manage definitons of 3000+ functions, and they don't need to be accessed externally,
// so everything is just in the .cpp file

struct LibraryV1 {
    LibraryV1(Core::Loader::SymbolsResolver* sym);

    LibcInternalMath m_libc_internal_math;
    LibcInternalStr m_libc_internal_str;
    LibcInternalMemory m_libc_internal_memory;
    LibcInternalIo m_libc_internal_io;
    LibcInternalThreads m_libc_internal_threads;
};

struct LibraryV2 {
    LibraryV2(Core::Loader::SymbolsResolver* sym);

    LibcInternalIoV2 m_libc_internal_io;
};

using Library = LibraryV2;

} // namespace Libraries::LibcInternal