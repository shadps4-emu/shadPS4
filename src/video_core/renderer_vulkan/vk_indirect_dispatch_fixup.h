// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "video_core/renderer_vulkan/vk_common.h"

namespace VideoCore {
class BufferCache;
}

namespace Vulkan {

class Instance;

// Multiplies the X component of a guest VkDispatchIndirectCommand by split_factor, since its
// group count lives in GPU memory and can't be scaled on the CPU like a direct dispatch's.
class IndirectDispatchFixup {
public:
    explicit IndirectDispatchFixup(const Instance& instance);
    ~IndirectDispatchFixup();

    // src_buffer/src_offset must already be safe to read. Returns the (buffer, offset) of the
    // patched command; the caller still needs its own barrier before using it as an indirect
    // dispatch source.
    std::pair<vk::Buffer, u32> Patch(vk::CommandBuffer cmdbuf, VideoCore::BufferCache& buffer_cache,
                                     vk::Buffer src_buffer, u32 src_offset, u32 split_factor);

private:
    const Instance& instance;
    vk::UniqueDescriptorSetLayout desc_layout;
    vk::UniquePipelineLayout pipeline_layout;
    vk::UniquePipeline pipeline;
};

} // namespace Vulkan
