// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <set>
#include <tsl/robin_map.h>

#include "common/types.h"
#include "video_core/texture_cache/image.h"

namespace VideoCore {
class TextureCache;
} // namespace VideoCore

namespace Vulkan {

class Instance;
class Scheduler;

/// Manages RT→alias VkImage synchronisation.
///
/// On PS4 (unified memory), different-format images at the same guest address share the
/// same physical backing and see each other's data with proper barriers.
/// In shadPS4 each view gets an independent VkImage, so we must explicitly copy GPU data
/// between them. Two models:
///
/// Pull (active) — CopyFromLastRt
///   Called from BindTextures when a consumer texture is about to be sampled.
///   If an RT was recorded at the same address (RecordRtWrite from BeginRendering),
///   the texture pulls (vkCmdCopyImage / vkCmdResolveImage) from that RT's VkImage
///   before sampling. Each (addr, tex_id) pair copies once per submit (dedup via
///   pending_rt_copied_).
///   Advantage: copy recorded before the consuming dispatch → GPU orders correctly.
///
/// Push (unused) — PushPendingRtAliases
///   Called from OnSubmit, iterates all pending RT addresses and pushes to each alias
///   found in the page_table. Avoids duplicate per-bind copies.
///   Remaining issue: OnSubmit is a post-execution hook — the GPU has already finished
///   the submitted commands. If a CS dispatch relies on alias data, the push arrives
///   too late (after the dispatch consumed old data). Mitigation would require pushing
///   before each dispatch or at RT-switch time, both of which add complexity without
///   clear benefit over the pull model.
///
/// Also handles 1×1 render-target readback (force-download so CPU can read the pixel).
class RenderTargetSync {
public:
    RenderTargetSync(const Instance& instance, Scheduler& scheduler,
                     VideoCore::TextureCache& texture_cache);
    ~RenderTargetSync();

    /// Records that an RT was written at this guest address.
    void RecordRtWrite(VAddr addr, VideoCore::ImageId id);

    /// If a larger-or-equal RT was recorded at the same address, copy its data into tex_id.
    /// Each (addr, tex_id) pair is only copied once per submit (de-duplicated).
    void CopyFromLastRt(VAddr addr, VideoCore::ImageId tex_id, u32 copy_w, u32 copy_h);

    /// Pushes all pending RT data to same-address alias images (push model, currently unused).
    void PushPendingRtAliases();

    /// Clears all pending records (called each submit).
    void ClearRecords();

    /// Schedules a 1×1 render target for guest-memory download.
    /// 1×1 RTs are used by games to communicate GPU results back to CPU
    /// (e.g. occlusion queries, visibility tests). Without a general RT→guest
    /// sync mechanism, we force-download individual 1×1 RTs so the CPU can
    /// read the pixel value from guest memory.
    void Schedule1x1Readback(VideoCore::ImageId image_id);

private:
    /// Pushes RT data to all same-address alias images found via page_table.
    void PushRtToAliases(VAddr addr, VideoCore::ImageId rt_id);

    /// Copies from rt_image to alias_image, handling cross-sample resolve.
    void CopyRtToAlias(VideoCore::Image& rt_image, VideoCore::Image& alias_image);

private:
    const Instance& instance;
    Scheduler& scheduler;
    VideoCore::TextureCache& texture_cache;

    tsl::robin_map<VAddr, VideoCore::ImageId> pending_rt_writes_;
    std::map<VAddr, std::set<VideoCore::ImageId>> pending_rt_copied_;
};

} // namespace Vulkan
