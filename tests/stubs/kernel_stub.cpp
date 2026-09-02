// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>

#include "tests/stubs/kernel_stub.h"

#include "core/libraries/kernel/kernel.h"
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

static u32 g_test_system_sw_version = CURRENT_FIRMWARE_VERSION;

void TestSetSystemSwVersion(u32 hex_version) {
    g_test_system_sw_version = hex_version;
}

void TestResetSystemSwVersion() {
    g_test_system_sw_version = CURRENT_FIRMWARE_VERSION;
}

s32 PS4_SYSV_ABI sceKernelGetSystemSwVersion(SwVersionStruct* ret) {
    if (ret == nullptr) {
        return 0;
    }
    const u32 fake_fw = g_test_system_sw_version;
    ret->hex_representation = fake_fw;
    std::snprintf(ret->text_representation, 28, "%2x.%03x.%03x", fake_fw >> 0x18,
                  fake_fw >> 0xc & 0xfff, fake_fw & 0xfff);
    return 0;
}

} // namespace Libraries::Kernel
