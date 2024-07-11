// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/kernel/event_queue.h"

namespace Libraries::Kernel {

using SceKernelUseconds = u32;
using SceKernelEqueue = EqueueInternal*;

int PS4_SYSV_ABI sceKernelCreateEqueue(SceKernelEqueue* eq, const char* name);
int PS4_SYSV_ABI sceKernelDeleteEqueue(SceKernelEqueue eq);
int PS4_SYSV_ABI sceKernelWaitEqueue(SceKernelEqueue eq, SceKernelEvent* ev, int num, int* out,
                                     SceKernelUseconds* timo);
void* PS4_SYSV_ABI sceKernelGetEventUserData(const SceKernelEvent* ev);
int PS4_SYSV_ABI sceKernelTriggerUserEvent(SceKernelEqueue eq, int id, void* udata);
int PS4_SYSV_ABI sceKernelDeleteUserEvent(SceKernelEqueue eq, int id);
int PS4_SYSV_ABI sceKernelAddUserEvent(SceKernelEqueue eq, int id);
int PS4_SYSV_ABI sceKernelAddUserEventEdge(SceKernelEqueue eq, int id);
s32 PS4_SYSV_ABI sceKernelAddHRTimerEvent(SceKernelEqueue eq, int id, timespec* ts, void* udata);

} // namespace Libraries::Kernel
