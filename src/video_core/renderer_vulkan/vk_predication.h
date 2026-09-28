// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>

#include "common/types.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace VideoCore {
class BufferCache;
}

namespace Vulkan {

class Instance;
class Scheduler;

/// Predication / ZPASS occlusion is disabled: packets always execute and draws are never
/// wrapped in occlusion queries or conditional rendering.
class PredicationManager {
public:
    explicit PredicationManager(const Instance&, Scheduler&, VideoCore::BufferCache&) {}

    void ControlZpassCounting() {}
    void ResetZpassCounting() {}
    void DumpZpassCounters(VAddr, u32) {}
    void EnableFromZpass(VAddr, u32, bool, bool) {}
    void EnableFromBool(VAddr, bool, bool, bool) {}
    void Disable() {}

    bool ShouldSkipPredicatedPacket() const {
        return false;
    }
    bool IsGpuPredicationActive() const {
        return false;
    }

    std::optional<u32> PrepareDrawQuery() {
        return std::nullopt;
    }
    void BeginDraw(vk::CommandBuffer, std::optional<u32>, bool) {}
    void EndDraw(vk::CommandBuffer, std::optional<u32>, bool) {}
};

} // namespace Vulkan
