#include "common/bit_field.h"
#include "src/common/enum.h"

#pragma once

union VariadicArgInfo {
    u32 raw;

    BitField<0, 12, u32> va_arg_idx;
};

union DebugPrintInfo {
    u32 raw;
    BitField<0, 16, u32> string_idx;
    BitField<16, 16, u32> num_args;
};