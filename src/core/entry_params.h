// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core {

struct EntryParams {
    int argc;
    u32 padding;
    const char* argv[33];
    VAddr entry_addr;
};

} // namespace Core
