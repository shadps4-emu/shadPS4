// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <set>
#include <tsl/robin_map.h>

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

/// Storage UAV ? sampled-alias sync, matching RenderTargetSync's pull model.
///
/// Sync() only records the write (O(1)). CopyFromLastWrite() runs from BindTextures
/// when a *sampled* texture at that address is bound, and copies a single mip0 slice
/// the size of the consumer — not a full-image CopyImage of every mip.
class StorageImageSync {
public:
    StorageImageSync(Scheduler& scheduler, Runtime& runtime, VideoCore::BufferCache& buffer_cache,
                     VideoCore::TextureCache& texture_cache);
    ~StorageImageSync();

    /// Record a storage-image write. No page-table walk, no GPU work.
    void Sync(VideoCore::ImageId image_id);

    /// Pull UAV data into a sampled alias. No-ops if this address was not written.
    void CopyFromLastWrite(VAddr addr, VideoCore::ImageId tex_id, u32 copy_w, u32 copy_h);

    void ClearRecords();

private:
    bool SkipSerial() const;
    void CopyToAlias(VideoCore::Image& src, VideoCore::Image& dst);
    void ScheduleAsyncGuestWrite(VideoCore::ImageId image_id);

private:
    Scheduler& scheduler;
    Runtime& runtime;
    VideoCore::BufferCache& buffer_cache;
    VideoCore::TextureCache& texture_cache;

    tsl::robin_map<VAddr, VideoCore::ImageId> pending_writes_;
    std::map<VAddr, std::set<VideoCore::ImageId>> pending_copied_;
};

} // namespace Vulkan
