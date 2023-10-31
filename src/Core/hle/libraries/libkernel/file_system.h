#pragma once
#include <types.h>
#include "core/PS4/Loader/SymbolsResolver.h"

namespace Core::Libraries::LibKernel {
int PS4_SYSV_ABI sceKernelOpen(const char *path, int flags, /* SceKernelMode*/ u16 mode);

// posix file system
int PS4_SYSV_ABI open(const char *path, int flags, /* SceKernelMode*/ u16 mode);


void fileSystemSymbolsRegister(SymbolsResolver *sym);
}  // namespace Core::Libraries::LibKernel