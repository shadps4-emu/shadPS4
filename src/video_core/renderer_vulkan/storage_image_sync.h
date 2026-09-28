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

class Runtime;
class Scheduler;

/// After a CS dispatch writes a storage VkImage, downloads (and re-tiles) into guest memory
/// so alias textures that refresh from guest see the UAV result.
class StorageImageSync {
public:
    StorageImageSync(Scheduler& scheduler, Runtime& runtime, VideoCore::BufferCache& buffer_cache,
                     VideoCore::TextureCache& texture_cache);
    ~StorageImageSync();

    void Sync(VideoCore::ImageId image_id);
    void ClearRecords() {}

private:
    bool HasAliasAtAddress(VAddr addr, VideoCore::ImageId self_id) const;

private:
    Scheduler& scheduler;
    Runtime& runtime;
    VideoCore::BufferCache& buffer_cache;
    VideoCore::TextureCache& texture_cache;
};

} // namespace Vulkan
