// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/slot_vector.h"
#include "common/types.h"
#include "video_core/texture_cache/image.h"

namespace VideoCore {
class BufferCache;
class TextureCache;
} // namespace VideoCore

namespace Vulkan {

class Scheduler;

/// Syncs compute storage image output to guest memory.
/// After a CS dispatch writes to a storage VkImage, this copies the image
/// data to a staging buffer, waits for GPU completion, and writes it back
/// to guest memory so that downstream texture consumers can read it.
class StorageImageSync {
public:
    StorageImageSync(Scheduler& scheduler, VideoCore::BufferCache& buffer_cache,
                     VideoCore::TextureCache& texture_cache);
    ~StorageImageSync();

    /// Copy storage image to staging buffer, wait for GPU, write to guest memory.
    void Sync(VideoCore::ImageId image_id);

private:
    Scheduler& scheduler;
    VideoCore::BufferCache& buffer_cache;
    VideoCore::TextureCache& texture_cache;
};

} // namespace Vulkan
