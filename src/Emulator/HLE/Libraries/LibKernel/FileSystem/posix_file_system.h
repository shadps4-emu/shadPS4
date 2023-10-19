#pragma once
#include "types.h"

namespace Emulator::HLE::Libraries::LibKernel::FileSystem::POSIX {
int PS4_SYSV_ABI open(const char *path, int flags, /* SceKernelMode*/ u16 mode);
}