// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <random>

#include "srandom_device.h"

namespace Core::Devices {

SRandomDevice::SRandomDevice() : gen(std::random_device{}()), dist(0, 255) {}

SRandomDevice::~SRandomDevice() = default;

s64 SRandomDevice::read(void* buf, u64 count) {
    const auto rbuf = static_cast<u8*>(buf);
    for (size_t i = 0; i < count; i++) {
        rbuf[i] = static_cast<u8>(dist(gen));
    }
    descriptor_offset += count;
    return count;
}

s64 SRandomDevice::write(const void* buf, u64 count) {
    descriptor_offset += count;
    return count;
}

} // namespace Core::Devices