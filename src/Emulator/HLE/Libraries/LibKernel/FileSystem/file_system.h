#pragma once
#include <types.h>

namespace Emulator::HLE::Libraries::LibKernel::FileSystem {
int PS4_SYSV_ABI sceKernelOpen(const char *path, int flags, /* SceKernelMode*/ u16 mode);

}