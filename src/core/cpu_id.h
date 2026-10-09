// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Xbyak {
class CodeGenerator;
}

namespace Core {

enum class CpuIdInstruction : u32 { Cpuid, Rdtscp, Rdpid };

void InitializeCpuId();
void EnableCpuIdFaulting();
bool HandleCpuIdFault(void* context, void* fault_address);
void SetCpuIdGuestAddressRange(uintptr_t begin, uintptr_t end);
void GenerateCpuIdInstruction(Xbyak::CodeGenerator& code, CpuIdInstruction instruction,
                              u32 destination = 0, bool fault_entry = false);

} // namespace Core
