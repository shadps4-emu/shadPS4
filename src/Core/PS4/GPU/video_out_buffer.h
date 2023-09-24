#pragma once

#include <types.h>

namespace GPU {

enum class VideoOutBufferFormat : u64 {
    Unknown,
    R8G8B8A8Srgb,
    B8G8R8A8Srgb,
};

class VideoOutBufferObj {
  public:
    explicit VideoOutBufferObj(VideoOutBufferFormat pixel_format, u32 width, u32 height, bool is_tiled, bool is_neo, u32 pitch) {
    }
};
}
