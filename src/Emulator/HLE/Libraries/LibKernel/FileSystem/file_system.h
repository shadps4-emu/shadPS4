#pragma once
#include <types.h>

namespace Emulator::HLE::Libraries::LibKernel::FileSystem {

constexpr u32 SCE_KERNEL_O_CREAT = 0x0200;
constexpr u32 SCE_KERNEL_O_DIRECTORY = 0x00020000;

int PS4_SYSV_ABI sceKernelOpen(const char *path, int flags, /* SceKernelMode*/ u16 mode);

}  // namespace Emulator::HLE::Libraries::LibKernel::FileSystem