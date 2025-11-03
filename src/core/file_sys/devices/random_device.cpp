// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <ctime>

#include "random_device.h"

namespace Core::Devices {

RandomDevice::RandomDevice() {
    std::srand(std::time(nullptr));
}

RandomDevice::~RandomDevice() = default;

s64 RandomDevice::pread(void* buf, u64 count, s64 offset) {
    auto rbuf = static_cast<char*>(buf);
    for (size_t i = 0; i < count; i++) {
        rbuf[i] = std::rand() & 0xFF;
    }
    return count;
}

s64 RandomDevice::pwrite(const void* buf, u64 count, s64 offset) {
    return count;
}

} // namespace Core::Devices