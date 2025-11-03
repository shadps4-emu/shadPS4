// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <ctime>

#include "srandom_device.h"

namespace Core::Devices {

SRandomDevice::SRandomDevice() {
    std::srand(std::time(nullptr));
}

SRandomDevice::~SRandomDevice() = default;

s64 SRandomDevice::pread(void* buf, size_t count, s64 offset) {
    auto rbuf = static_cast<char*>(buf);
    for (size_t i = 0; i < count; i++) {
        rbuf[i] = std::rand();
    }
    return count;
}

s64 SRandomDevice::pwrite(const void* buf, size_t count, s64 offset) {
    return count;
}

} // namespace Core::Devices