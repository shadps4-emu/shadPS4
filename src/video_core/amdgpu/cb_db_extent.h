// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace AmdGpu {

union CbDbExtent {
    struct {
        u16 width;
        u16 height;
    };
    u32 raw;

    bool Valid() const {
        return raw != 0;
    }
};

} // namespace AmdGpu
