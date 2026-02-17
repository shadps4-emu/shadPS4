// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <random>

#include "random_device.h"

namespace Core::Devices {

RandomDevice::RandomDevice() : gen(std::random_device{}()), dist(0, 255) {}

RandomDevice::~RandomDevice() = default;

s64 RandomDevice::read(void* buf, u64 count) {
    const auto rbuf = static_cast<u8*>(buf);
    for (size_t i = 0; i < count; i++) {
        rbuf[i] = static_cast<u8>(dist(gen));
    }
    descriptor_offset += count;
    return count;
}

s64 RandomDevice::write(const void* buf, u64 count) {
    descriptor_offset += count;
    return count;
}

} // namespace Core::Devices