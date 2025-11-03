// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include "null_device.h"

namespace Core::Devices {

NullDevice::NullDevice() = default;
NullDevice::~NullDevice() = default;

s64 NullDevice::pread(void* buf, size_t count, u64 offset) {
    return 0;
}

s64 NullDevice::pwrite(const void* buf, size_t count, u64 offset) {
    return count;
}

} // namespace Core::Devices