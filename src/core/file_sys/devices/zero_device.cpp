// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include "zero_device.h"

namespace Core::Devices {

ZeroDevice::ZeroDevice() = default;
ZeroDevice::~ZeroDevice() = default;

s64 ZeroDevice::read(void* buf, u64 count) {
    memset(buf, 0, count);
    return count;
}

s64 ZeroDevice::write(const void* buf, u64 count) {
    return count;
}

s64 ZeroDevice::pread(void* buf, size_t count, u64 offset) {
    memset(buf, 0, count);
    return count;
}

s64 ZeroDevice::pwrite(const void* buf, size_t count, u64 offset) {
    return count;
}

} // namespace Core::Devices