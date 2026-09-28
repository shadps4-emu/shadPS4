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

/// Syncs compute storage image output to guest memory.
/// After a CS dispatch writes to a storage VkImage, this copies the image
/// data to a staging buffer, waits for GPU completion, and writes it back
/// to guest memory so that downstream texture consumers can read it.
///
/// The guest write-back is only useful when something can actually observe it:
/// either another currently-resident VkImage aliasing the same guest address
/// (same pull/push philosophy as RenderTargetSync), or genuine CPU consumption.
/// Forcing a full CPU/GPU pipeline stall (Map + wait-for-GPU + write-back) after
/// *every* storage-image dispatch — even when nothing aliases that address —
/// serializes the CPU and GPU and was responsible for large performance
/// regressions in compute-heavy titles. Sync() now cheaply checks the page
/// table first and skips the expensive stall entirely when there is no
/// known alias image at that address.
class StorageImageSync {
public:
    StorageImageSync(Scheduler& scheduler, Runtime& runtime, VideoCore::BufferCache& buffer_cache,
                     VideoCore::TextureCache& texture_cache);
    ~StorageImageSync();

    /// Copy storage image to staging buffer, wait for GPU, write to guest memory.
    /// No-ops cheaply if no other image is currently aliasing this guest address.
    void Sync(VideoCore::ImageId image_id);

private:
    /// Returns true if some other resident image shares this guest address,
    /// meaning the guest write-back can actually be observed by something.
    bool HasAliasAtAddress(VAddr addr, VideoCore::ImageId self_id) const;

private:
    Scheduler& scheduler;
    Runtime& runtime;
    VideoCore::BufferCache& buffer_cache;
    VideoCore::TextureCache& texture_cache;
};

} // namespace Vulkan