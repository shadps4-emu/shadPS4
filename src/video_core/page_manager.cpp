// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include <boost/icl/interval_set.hpp>
#include "common/alignment.h"
#include "common/assert.h"
#include "common/error.h"
#include "core/signals.h"
#include "video_core/page_manager.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"

#ifndef _WIN64
#include <sys/mman.h>
#else
#include <windows.h>
#endif

namespace VideoCore {

constexpr size_t PAGESIZE = 4_KB;
constexpr size_t PAGEBITS = 12;

struct PageManager::Impl {
    Impl(Vulkan::Rasterizer* rasterizer_) {
        rasterizer = rasterizer_;

        // Should be called first.
        constexpr auto priority = std::numeric_limits<u32>::min();
        Core::Signals::Instance()->RegisterAccessViolationHandler(GuestFaultSignalHandler,
                                                                  priority);
    }

    void OnMap(VAddr address, size_t size) {
        owned_ranges += boost::icl::interval<VAddr>::right_open(address, address + size);
    }

    void OnUnmap(VAddr address, size_t size) {
        owned_ranges -= boost::icl::interval<VAddr>::right_open(address, address + size);
    }

    void Protect(VAddr address, size_t size, bool allow_write) {
#ifdef _WIN32
        DWORD prot = allow_write ? PAGE_READWRITE : PAGE_READONLY;
        DWORD old_prot{};
        BOOL result = VirtualProtect(std::bit_cast<LPVOID>(address), size, prot, &old_prot);
        ASSERT_MSG(result != 0, "Region protection failed");
#else
        mprotect(reinterpret_cast<void*>(address), size,
                 PROT_READ | (allow_write ? PROT_WRITE : 0));
#endif
    }

    static bool GuestFaultSignalHandler(void* code_address, void* fault_address, bool is_write) {
        const auto addr = reinterpret_cast<VAddr>(fault_address);
        if (is_write && owned_ranges.find(addr) != owned_ranges.end()) {
            const VAddr addr_aligned = Common::AlignDown(addr, PAGESIZE);
            rasterizer->InvalidateMemory(addr_aligned, PAGESIZE);
            return true;
        }
        return false;
    }

    inline static Vulkan::Rasterizer* rasterizer;
    inline static boost::icl::interval_set<VAddr> owned_ranges;
};

PageManager::PageManager(Vulkan::Rasterizer* rasterizer_)
    : impl{std::make_unique<Impl>(rasterizer_)}, rasterizer{rasterizer_} {}

PageManager::~PageManager() = default;

void PageManager::OnGpuMap(VAddr address, size_t size) {
    impl->OnMap(address, size);
}

void PageManager::OnGpuUnmap(VAddr address, size_t size) {
    impl->OnUnmap(address, size);
}

void PageManager::UpdatePagesCachedCount(VAddr addr, u64 size, s32 delta) {
    static constexpr u64 PageShift = 12;

    std::scoped_lock lk{mutex};
    const u64 num_pages = ((addr + size - 1) >> PageShift) - (addr >> PageShift) + 1;
    const u64 page_start = addr >> PageShift;
    const u64 page_end = page_start + num_pages;

    const auto pages_interval =
        decltype(cached_pages)::interval_type::right_open(page_start, page_end);
    if (delta > 0) {
        cached_pages.add({pages_interval, delta});
    }

    const auto& range = cached_pages.equal_range(pages_interval);
    for (const auto& [range, count] : boost::make_iterator_range(range)) {
        const auto interval = range & pages_interval;
        const VAddr interval_start_addr = boost::icl::first(interval) << PageShift;
        const VAddr interval_end_addr = boost::icl::last_next(interval) << PageShift;
        const u32 interval_size = interval_end_addr - interval_start_addr;
        if (delta > 0 && count == delta) {
            impl->Protect(interval_start_addr, interval_size, false);
        } else if (delta < 0 && count == -delta) {
            impl->Protect(interval_start_addr, interval_size, true);
        } else {
            ASSERT(count >= 0);
        }
    }

    if (delta < 0) {
        cached_pages.add({pages_interval, delta});
    }
}

} // namespace VideoCore
