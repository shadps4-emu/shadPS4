// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>
#include <boost/asio/steady_timer.hpp>

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

enum AioState {
    ORBIS_KERNEL_AIO_STATE_SUBMITTED = 1,
    ORBIS_KERNEL_AIO_STATE_PROCESSING = 2,
    ORBIS_KERNEL_AIO_STATE_COMPLETED = 3,
    ORBIS_KERNEL_AIO_STATE_ABORTED = 4
};

struct OrbisKernelAioResult {
    s64 returnValue;
    u32 state;
};

typedef s32 OrbisKernelAioSubmitId;

struct OrbisKernelAioRWRequest {
    s64 offset;
    s64 nbyte;
    void* buf;
    OrbisKernelAioResult* result;
    s32 fd;
};

void RegisterAio(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Kernel