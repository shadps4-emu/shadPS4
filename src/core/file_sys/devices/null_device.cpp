// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include "null_device.h"

namespace Core::Devices {

NullDevice::NullDevice() = default;
NullDevice::~NullDevice() = default;

s64 NullDevice::read(void* buf, u64 count) {
    return 0;
}

s64 NullDevice::write(const void* buf, u64 count) {
    return count;
}

} // namespace Core::Devices