// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

namespace VideoCore {
struct Image;
class TextureCache;
} // namespace VideoCore

namespace Vulkan {

class Instance;
class Scheduler;

/// Manages async GPU→guest-memory downloads for compute shader output.
/// Only active for God of War III (CUSA01623 / CUSA01715 / CUSA01740).
class ComputeDownloadManager {
public:
    ComputeDownloadManager(const Instance& instance, Scheduler& scheduler,
                           VideoCore::TextureCache& texture_cache);
    ~ComputeDownloadManager();

    /// True for known GOW3 titles that use compute→texture aliasing.
    static bool IsEnabled();

    /// Initiate async download for one storage image written by a direct dispatch.
    void SyncOne(VideoCore::Image& storage_img, u32 grid_x, u32 grid_y);

    /// Resolve all pending downloads. Must be called before BindTextures.
    void ResolvePending();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Vulkan
