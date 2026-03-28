// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/va_ctx.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "libc_internal.h"
#include "libc_internal_io.h"
#include "libc_internal_math.h"
#include "libc_internal_memory.h"
#include "libc_internal_str.h"
#include "libc_internal_threads.h"
#include "printf.h"

namespace Libraries::LibcInternal {

EngineV1::EngineV1(Core::Loader::SymbolsResolver* sym)
    : m_math_engine(sym), m_str_engine(sym), m_memory_engine(sym), m_io_engine(sym),
      m_threads_engine(sym) {}

// Used to forcibly enable HLEs for broken LLE functions.
EngineV2::EngineV2(Core::Loader::SymbolsResolver* sym) : m_io_engine(sym) {}

} // namespace Libraries::LibcInternal