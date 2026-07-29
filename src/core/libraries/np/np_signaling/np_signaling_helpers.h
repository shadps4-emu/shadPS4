// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Np::NpSignaling::Helpers {

struct SignalingRuntimeHooks {
    void (*start_dispatch)(s32 priority, u64 affinity_mask, u64 stack_size);
    void (*start_receive)(s32 priority, u64 affinity_mask, u64 stack_size);
    void (*start_ping)(s32 priority, u64 affinity_mask, u64 stack_size);
    void (*stop_ping)();
    void (*stop_receive)();
    void (*stop_dispatch)();
};

void SetRuntimeHooks(const SignalingRuntimeHooks& hooks);
s32 CheckInitializeAppType(u32* is_app_type_4);
s32 InitSignalingHeap(s64 pool_size);
void ShutdownSignalingHeap();
s32 CheckAppType();
s32 StartMainRuntime(s32 thread_priority, s32 cpu_affinity_mask, s64 thread_stack_size);
s32 StartEchoRuntime(s32 thread_priority, s32 cpu_affinity_mask);
void ShutdownRuntime();

} // namespace Libraries::Np::NpSignaling::Helpers

namespace Libraries::Np::NpSignaling {

void RegisterRuntimeHooks();

}
