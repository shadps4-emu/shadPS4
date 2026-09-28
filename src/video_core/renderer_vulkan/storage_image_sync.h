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

/// Syncs compute storage image output to alias VkImages (and, when required, guest memory).
///
/// After a CS dispatch writes a storage image, Sync() only records that write. Consumers pull
/// GPU copies in BindTextures via CopyFromLastWrite — the same model as RenderTargetSync.
/// That avoids a CPU/GPU stall (scheduler.Finish) after every storage dispatch.
///
/// Titles that disable the alias check still need a guest write-back; that path downloads and
/// tiles asynchronously after the GPU tick, never Finish() on the emulation thread.
class StorageImageSync {
public:
    StorageImageSync(Scheduler& scheduler, Runtime& runtime, VideoCore::BufferCache& buffer_cache,
                     VideoCore::TextureCache& texture_cache);
    ~StorageImageSync();

    /// Record a storage-image write. Cheap; no GPU wait.
    void Sync(VideoCore::ImageId image_id);

    /// If a storage image was written at this address, GPU-copy it into tex_id.
    void CopyFromLastWrite(VAddr addr, VideoCore::ImageId tex_id, u32 copy_w, u32 copy_h);

    /// Drop recorded writes at submit time (mirrors RenderTargetSync).
    void ClearRecords();

private:
    bool SkipSerial() const;
    bool HasAliasAtAddress(VAddr addr, VideoCore::ImageId self_id) const;
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
