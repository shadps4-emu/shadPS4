#pragma once

#include "Objects/event_queue.h"

namespace HLE::Libs::LibKernel::EventQueues {
using SceKernelUseconds = u32;
using SceKernelEqueue = Kernel::Objects::EqueueInternal*;

int PS4_SYSV_ABI sceKernelCreateEqueue(SceKernelEqueue* eq, const char* name);
int PS4_SYSV_ABI sceKernelWaitEqueue(SceKernelEqueue eq, HLE::Kernel::Objects::SceKernelEvent* ev, int num, int* out, SceKernelUseconds *timo);

};  // namespace HLE::Libs::LibKernel::EventQueues
