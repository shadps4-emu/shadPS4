#pragma once

#include "common/types.h"

#include "gpu_memory.h"

namespace GPU {

enum class VideoOutBufferFormat : u64 {
    Unknown,
    R8G8B8A8Srgb,
    B8G8R8A8Srgb,
};

class VideoOutBufferObj : public GPUObject {
public:
    static constexpr int PIXEL_FORMAT_PARAM = 0;
    static constexpr int WIDTH_PARAM = 1;
    static constexpr int HEIGHT_PARAM = 2;
    static constexpr int IS_TILE_PARAM = 3;
    static constexpr int IS_NEO_PARAM = 4;
    static constexpr int PITCH_PARAM = 5;

    explicit VideoOutBufferObj(VideoOutBufferFormat pixel_format, u32 width, u32 height,
                               bool is_tiled, bool is_neo, u32 pitch) {
        obj_params[PIXEL_FORMAT_PARAM] = static_cast<uint64_t>(pixel_format);
        obj_params[WIDTH_PARAM] = width;
        obj_params[HEIGHT_PARAM] = height;
        obj_params[IS_TILE_PARAM] = is_tiled ? 1 : 0;
        obj_params[IS_NEO_PARAM] = is_neo ? 1 : 0;
        obj_params[PITCH_PARAM] = pitch;
        check_hash = true;
        objectType = GPU::MemoryObjectType::VideoOutBufferObj;
    }

    create_func_t getCreateFunc() const override;
    update_func_t getUpdateFunc() const override;
};
} // namespace GPU
