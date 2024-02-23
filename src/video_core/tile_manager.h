#pragma once

#include "common/types.h"

namespace VideoCore {

void ConvertTileToLinear(void* dst, const void* src, u32 width, u32 height, bool neo);

} // namespace VideoCore
