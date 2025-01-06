// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "video_core/buffer_cache/buffer.h"

namespace VideoCore {

class TextureCache;
struct ImageInfo;

enum DetilerType : u32 {
    Micro8,
    Micro16,
    Micro32,
    Micro64,
    Micro128,

    Macro8,
    Macro32,
    Macro64,

    Max
};

struct DetilerContext {
    vk::UniquePipeline pl;
    vk::UniquePipelineLayout pl_layout;
};

class TileManager {
public:
    using ScratchBuffer = std::pair<vk::Buffer, VmaAllocation>;

    TileManager(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler);
    ~TileManager();

    std::pair<vk::Buffer, u32> TryDetile(vk::Buffer in_buffer, u32 in_offset,
                                         const ImageInfo& info);

    ScratchBuffer AllocBuffer(u32 size, bool is_storage = false);
    void Upload(ScratchBuffer buffer, const void* data, size_t size);
    void FreeBuffer(ScratchBuffer buffer);

private:
    const DetilerContext* GetDetiler(const ImageInfo& info) const;

private:
    const Vulkan::Instance& instance;
    Vulkan::Scheduler& scheduler;
    vk::UniqueDescriptorSetLayout desc_layout;
    std::array<DetilerContext, DetilerType::Max> detilers;
};

} // namespace VideoCore
