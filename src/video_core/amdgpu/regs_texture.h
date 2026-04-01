// SPDX-FileCopyrightText: Copyright 2025 shadBloodborne Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <bit>
#include "common/types.h"

namespace AmdGpu {

struct BorderColorBuffer {
    u64 base_addr : 40;

    template <typename T = VAddr>
    const T Address() const {
        return std::bit_cast<T>(base_addr << 8);
    }
};

} // namespace AmdGpu
