// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/kernel/threads/pthread.h"

namespace Libraries::Kernel {

HleThreads::HleThreads(Core::Loader::SymbolsResolver* sym)
    : m_mutex(sym), m_cond(sym), m_rw_lock(sym), m_semaphore(sym), m_spec(sym), m_thread_attr(sym),
      m_thread(sym), m_rtld(sym), m_pthread_clean(sym) {}

} // namespace Libraries::Kernel
