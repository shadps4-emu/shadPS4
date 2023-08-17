#pragma once

#include <types.h>
#include "Objects/event_queue.h"

namespace HLE::Libs::LibKernel::EventQueues {
using SceKernelEqueue = Kernel::Objects::EqueueInternal*;

int PS4_SYSV_ABI sceKernelCreateEqueue(SceKernelEqueue* eq, const char* name);
};  // namespace HLE::Libs::LibKernel::EventQueues