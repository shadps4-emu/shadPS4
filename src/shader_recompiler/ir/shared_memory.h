// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstddef>
#include "common/bit_field.h"
#include "common/types.h"

namespace Shader::SharedMemory {

union Flags {
    Flags(bool _is_gds, bool _is_scratch) {
        is_gds.Assign(_is_gds), is_scratch.Assign(_is_scratch);
    }
    Flags() : Flags(false, false) {}

    BitField<0, 1, u64> is_gds;
    BitField<1, 1, u64> is_scratch;
    u64 raw;
};

} // namespace Shader::SharedMemory
