#pragma once

#include "types.h"

namespace GPU {

void convertTileToLinear(void* dst, const void* src, u32 width, u32 height, bool neo);
}