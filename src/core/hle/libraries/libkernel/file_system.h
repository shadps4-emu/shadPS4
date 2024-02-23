#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibKernel {

struct SceKernelIovec {
    void* iov_base;
    size_t iov_len;
};

int PS4_SYSV_ABI sceKernelOpen(const char* path, int flags, /* SceKernelMode*/ u16 mode);

int PS4_SYSV_ABI posix_open(const char* path, int flags, /* SceKernelMode*/ u16 mode);

void fileSystemSymbolsRegister(Loader::SymbolsResolver* sym);

} // namespace Core::Libraries::LibKernel
