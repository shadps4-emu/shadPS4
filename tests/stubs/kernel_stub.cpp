// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "tests/stubs/kernel_stub.h"

#include "core/libraries/kernel/process.h"

namespace Libraries::Kernel {

static constexpr s32 DefaultTestSdkVersion = 0x4500000;
static s32 g_test_sdk_version = DefaultTestSdkVersion;

void TestSetSdkVersion(s32 ver) {
    g_test_sdk_version = ver;
}

void TestResetSdkVersion() {
    g_test_sdk_version = DefaultTestSdkVersion;
}

s32 PS4_SYSV_ABI sceKernelGetCompiledSdkVersion(s32* ver) {
    if (ver) {
        *ver = g_test_sdk_version;
    }
    return 0;
}

} // namespace Libraries::Kernel
