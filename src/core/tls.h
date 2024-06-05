// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Xbyak {
class CodeGenerator;
}

namespace Core {

union DtvEntry {
    size_t counter;
    void* pointer;
};

struct Tcb {
    Tcb* tcb_self;
    DtvEntry* tcb_dtv;
    void* tcb_thread;
};

/// Sets the data pointer to the TCB block.
void SetTcbBase(void* image_address);

/// Retrieves Tcb structure for the calling thread.
Tcb* GetTcbBase();

/// Patches any instructions that access guest TLS to use provided storage.
void PatchTLS(u64 segment_addr, u64 segment_size, Xbyak::CodeGenerator& c);

} // namespace Core
