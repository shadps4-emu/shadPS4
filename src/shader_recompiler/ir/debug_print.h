#include "common/bit_field.h"
#include "src/common/enum.h"

#pragma once

union VariadicArgInfo {
    u32 raw;

    BitField<0, 12, u32> va_arg_idx;
};