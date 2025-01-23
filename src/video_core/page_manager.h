// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <boost/icl/interval_map.hpp>
#ifdef __linux__
#include "common/adaptive_mutex.h"
#endif
#include "common/spin_lock.h"
#include "common/types.h"

namespace Vulkan {
class Rasterizer;
}

namespace VideoCore {

class PageManager {
public:
    explicit PageManager(Vulkan::Rasterizer* rasterizer);
    ~PageManager();

    /// Register a range of mapped gpu memory.
    void OnGpuMap(VAddr address, size_t size);

    /// Unregister a range of gpu memory that was unmapped.
    void OnGpuUnmap(VAddr address, size_t size);

    /// Increase/decrease the number of surface in pages touching the specified region
    void UpdatePagesCachedCount(VAddr addr, u64 size, s32 delta);

    static VAddr GetPageAddr(VAddr addr);
    static VAddr GetNextPageAddr(VAddr addr);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    Vulkan::Rasterizer* rasterizer;
    boost::icl::interval_map<VAddr, s32> cached_pages;
#ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
    Common::AdaptiveMutex lock;
#else
    Common::SpinLock lock;
#endif
};

} // namespace VideoCore
