// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstddef>
#include "common/bit_field.h"
#include "common/types.h"

namespace Shader::SharedMemory {

enum class Section : u32 {
    Guest = 0, // intentionally default
    Scratch = 1,
    NumSections
};

static constexpr auto NumSections = static_cast<size_t>(Section::NumSections);

union Flags {
    Flags(bool _is_gds, Section _section) {
        is_gds.Assign(_is_gds), section.Assign(_section);
    }
    Flags() : Flags(false, Section::Guest) {}
    ~Flags() = default;

    BitField<0, 1, u64> is_gds;
    BitField<16, 16, Section> section;
    u64 raw;
};

} // namespace Shader::SharedMemory
