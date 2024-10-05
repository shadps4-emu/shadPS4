#include "common/bit_field.h"
#include "src/common/types.h"

#pragma once

union DebugPrintFlags {
    u32 raw;
    BitField<0, 16, u32> string_idx;
    BitField<16, 16, u32> num_args;
};