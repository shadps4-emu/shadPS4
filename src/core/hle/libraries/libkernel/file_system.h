#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibKernel {

int PS4_SYSV_ABI sceKernelOpen(const char *path, int flags, /* SceKernelMode*/ u16 mode);

int PS4_SYSV_ABI open(const char *path, int flags, /* SceKernelMode*/ u16 mode);

void fileSystemSymbolsRegister(Loader::SymbolsResolver *sym);

} // namespace Core::Libraries::LibKernel
