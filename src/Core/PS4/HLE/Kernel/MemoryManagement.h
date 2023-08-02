#pragma once

#include <types.h>

namespace HLE::Libs::LibKernel::MemoryManagement {

u64 PS4_SYSV_ABI sceKernelGetDirectMemorySize();
int PS4_SYSV_ABI sceKernelAllocateDirectMemory(s64 searchStart, s64 searchEnd, u64 len, u64 alignment, int memoryType, s64* physAddrOut);

};  // namespace HLE::Libs::LibKernel::MemoryManagement