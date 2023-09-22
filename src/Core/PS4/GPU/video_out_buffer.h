#pragma once

#include <types.h>

namespace GPU {

enum class VideoOutBufferFormat : u64 {
    Unknown,
    R8G8B8A8Srgb,
    B8G8R8A8Srgb,
};
}
