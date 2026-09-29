// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/container/small_vector.hpp>
#include "common/assert.h"
#include "common/config.h"
#include "common/debug.h"
#include "common/div_ceil.h"
#include "common/memory_patcher.h"
#include "common/range_lock.h"
#include "common/signal_context.h"
#include "core/memory.h"
#include "core/signals.h"
#include "video_core/page_manager.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"

#ifndef _WIN64
#include <sys/mman.h>
#include "common/adaptive_mutex.h"
#ifdef ENABLE_USERFAULTFD
#include <thread>
#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include "common/error.h"
#endif
#else
#include <windows.h>
#include "common/spin_lock.h"
#endif

#ifdef __linux__
#include "common/adaptive_mutex.h"
#else
#include "common/spin_lock.h"
#endif

namespace VideoCore {

constexpr size_t PAGE_SIZE = 4_KB;
constexpr size_t PAGE_BITS = 12;

struct PageManager::Impl {
    struct PageState {
        u8 num_watchers{};
        u8 num_write_watchers{};
        u8 num_read_watchers{};

        bool IsFastPathGame() const noexcept {
            const auto& serial = MemoryPatcher::g_game_serial;
            return (serial == "CUSA03173" || serial == "CUSA00900" || serial == "CUSA00208" ||
                    serial == "CUSA01363" || serial == "CUSA01322" || serial == "CUSA003027" ||
                    serial == "CUSA00299" || serial == "CUSA00207" || serial == "CUSA03014" ||
                    serial == "CUSA03023" || serial == "CUSA50617" || serial == "CUSA18723" ||
                    serial == "CUSA28863");
        }

        Core::MemoryPermission WritePerm() const noexcept {
            if (IsFastPathGame()) {
                return num_watchers == 0 ? Core::MemoryPermission::Write
                                         : Core::MemoryPermission::Read;
            } else {
                return num_write_watchers == 0 ? Core::MemoryPermission::Write
                                               : Core::MemoryPermission::None;
            }
        }

        Core::MemoryPermission ReadPerm() const noexcept {
            return num_read_watchers == 0 ? Core::MemoryPermission::Read
                                          : Core::MemoryPermission::None;
        }

        Core::MemoryPermission Perms() const noexcept {
            return ReadPerm() | WritePerm();
        }

        template <s32 delta, bool is_read = false>
        u8 AddDelta() {
            if (IsFastPathGame()) {
                if constexpr (delta == 1) {
                    return ++num_watchers;
                } else if constexpr (delta == -1) {
                    ASSERT_MSG(num_watchers > 0, "Not enough watchers");
                    return --num_watchers;
                } else {
                    return num_watchers;
                }
            } else {
                if constexpr (is_read) {
                    if constexpr (delta == 1) {
                        return ++num_read_watchers;
                    } else if constexpr (delta == -1) {
                        ASSERT_MSG(num_read_watchers > 0, "Not enough watchers");
                        return --num_read_watchers;
                    } else {
                        return num_read_watchers;
                    }
                } else {
                    if constexpr (delta == 1) {
                        return ++num_write_watchers;
                    } else if constexpr (delta == -1) {
                        ASSERT_MSG(num_write_watchers > 0, "Not enough watchers");
                        return --num_write_watchers;
                    } else {
                        return num_write_watchers;
                    }
                }
            }
        }
    };

    static constexpr size_t ADDRESS_BITS = 40;
    static constexpr size_t NUM_ADDRESS_PAGES = 1ULL << (40 - PAGE_BITS);
    static constexpr size_t NUM_ADDRESS_LOCKS = NUM_ADDRESS_PAGES / PAGES_PER_LOCK;
    inline static Vulkan::Rasterizer* rasterizer;

#ifdef ENABLE_USERFAULTFD
    Impl(Vulkan::Rasterizer* rasterizer_) {
        rasterizer = rasterizer_;
        uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK | UFFD_USER_MODE_ONLY);
        ASSERT_MSG(uffd != -1, "{}", Common::GetLastErrorMsg());

        uffdio_api api;
        api.api = UFFD_API;
        api.features = UFFD_FEATURE_THREAD_ID;
        const int ret = ioctl(uffd, UFFDIO_API, &api);
        ASSERT(ret == 0 && api.api == UFFD_API);

        ufd_thread = std::jthread([&](std::stop_token token) { UffdHandler(token); });
    }

    void OnMap(VAddr address, size_t size) {
        uffdio_register reg;
        reg.range.start = address;
        reg.range.len = size;
        reg.mode = UFFDIO_REGISTER_MODE_WP;
        const int ret = ioctl(uffd, UFFDIO_REGISTER, &reg);
        ASSERT_MSG(ret != -1, "Uffdio register failed with error: {}", Common::GetLastErrorMsg());
    }

    void OnUnmap(VAddr address, size_t size) {
        uffdio_range range;
        range.start = address;
        range.len = size;
        const int ret = ioctl(uffd, UFFDIO_UNREGISTER, &range);
        ASSERT_MSG(ret != -1, "Uffdio unregister failed with error: {}", Common::GetLastErrorMsg());
    }

    void Protect(VAddr address, size_t size, Core::MemoryPermission perms) {
        bool allow_write = True(perms & Core::MemoryPermission::Write);
        uffdio_writeprotect wp;
        wp.range.start = address;
        wp.range.len = size;
        wp.mode = allow_write ? 0 : UFFDIO_WRITEPROTECT_MODE_WP;
        const int ret = ioctl(uffd, UFFDIO_WRITEPROTECT, &wp);
        ASSERT_MSG(ret != -1, "Uffdio writeprotect failed: {}", Common::GetLastErrorMsg());
    }

    void UffdHandler(std::stop_token token) {
        Common::SetCurrentThreadName("shadPS4:Uffd");

        auto regions = Core::Memory::Instance()->GetAddressSpace().GetUsableRegions();
        for (auto& region : regions) {
            OnMap(region.lower(), region.upper());
        }
        LOG_INFO(Common_Memory, "registered reserved memory with userfaultfd");

        while (!token.stop_requested()) {
            pollfd pollfd;
            pollfd.fd = uffd;
            pollfd.events = POLLIN;
            const int pollres = poll(&pollfd, 1, -1);
            if (pollres <= 0)
                continue;

            uffd_msg msg;
            const int readret = read(uffd, &msg, sizeof(msg));
            if (readret != sizeof(msg))
                continue;

            const VAddr addr = msg.arg.pagefault.address;
            const auto ptid = msg.arg.pagefault.feat.ptid;
            rasterizer->InvalidateMemory(addr, 1,
                                         ptid == rasterizer->GetGpuCommandProcessorThreadId());

            // Some calls to InvalidateMemory never reach the UFFDIO_WRITEPROTECT ioctl in
            // ::Protect, therefore we use MODE_DONTWAKE and wake the thread with UFFDIO_WAKE here
            uffdio_range wake;
            wake.start = msg.arg.pagefault.address;
            wake.len = PM_PAGE_SIZE;
            const int ret = ioctl(uffd, UFFDIO_WAKE, &wake);
            ASSERT_MSG(ret != -1, "Waking thread {} failed with: {}", ptid,
                       Common::GetLastErrorMsg());
        }
    }
    std::jthread ufd_thread;
    int uffd;
#else
    Impl(Vulkan::Rasterizer* rasterizer_) {
        rasterizer = rasterizer_;
        constexpr auto priority = std::numeric_limits<u32>::min();
        Core::Signals::Instance()->RegisterAccessViolationHandler(GuestFaultSignalHandler,
                                                                  priority);
    }

    void OnMap(VAddr address, size_t size) {}
    void OnUnmap(VAddr address, size_t size) {}

    void Protect(VAddr address, size_t size, Core::MemoryPermission perms) {
        RENDERER_TRACE;
        auto* memory = Core::Memory::Instance();
        auto& impl = memory->GetAddressSpace();
        ASSERT_MSG(perms != Core::MemoryPermission::Write,
                   "Attempted to protect region as write-only which is not a valid permission");
        impl.Protect(address, size, perms);
    }

    static bool GuestFaultSignalHandler(void* context, void* fault_address) {
        const auto addr = reinterpret_cast<VAddr>(fault_address);
        const auto is_gpu_thread =
            std::this_thread::get_id() == rasterizer->GetGpuCommandProcessorThread();
        if (Common::IsWriteError(context)) {
            return rasterizer->InvalidateMemory(addr, 8, is_gpu_thread);
        } else {
            return rasterizer->ReadMemory(addr, 8, is_gpu_thread);
        }
    }
#endif

    template <bool track, bool is_read>
    void UpdatePageWatchers(VAddr addr, u64 size) {
        RENDERER_TRACE;

        size_t page = addr >> PAGE_BITS;
        const u64 page_end = Common::DivCeil(addr + size, PAGE_SIZE);

        const auto lock_start = locks.begin() + (page / PAGES_PER_LOCK);
        const auto lock_end = locks.begin() + Common::DivCeil(page_end, PAGES_PER_LOCK);
        Common::RangeLockGuard lk(lock_start, lock_end);

        auto perms = cached_pages[page].Perms();
        u64 range_begin = page;
        u64 range_bytes = 0;
        u64 potential_range_bytes = 0;

        const auto release_pending = [&] {
            if (range_bytes > 0) {
                RENDERER_TRACE;

                Protect(range_begin << PAGE_BITS, range_bytes, perms);
                range_bytes = 0;
                potential_range_bytes = 0;
            }
        };

        for (; page != page_end; ++page) {
            PageState& state = cached_pages[page];
            const u8 new_count = state.AddDelta<track ? 1 : -1, is_read>();
            const auto new_perms = state.Perms();

            if (new_perms != perms) [[unlikely]] {
                release_pending();
                perms = new_perms;
            } else if (range_bytes != 0) {
                potential_range_bytes += PAGE_SIZE;
            }

            if ((new_count == 0 && !track) || (new_count == 1 && track)) {
                if (range_bytes == 0) {
                    range_begin = page;
                    potential_range_bytes = PAGE_SIZE;
                }
                range_bytes = potential_range_bytes;
            }
        }
        release_pending();
    }

    template <bool track, bool is_read>
    void UpdatePageWatchersForRegion(VAddr base_addr, RegionBits& mask) {
        RENDERER_TRACE;

        for (auto range : mask) {
            if (range.first == range.second)
                continue;
            const VAddr start_addr = base_addr + (range.first << PAGE_BITS);
            const u64 size = (range.second - range.first) << PAGE_BITS;
            UpdatePageWatchers<track, is_read>(start_addr, size);
        }
    }

    std::array<PageState, NUM_ADDRESS_PAGES> cached_pages{};
#ifdef __linux__
    using LockType = Common::AdaptiveMutex;
#else
    using LockType = Common::SpinLock;
#endif
    std::array<LockType, NUM_ADDRESS_LOCKS> locks{};
};

PageManager::PageManager(Vulkan::Rasterizer* rasterizer_)
    : impl{std::make_unique<Impl>(rasterizer_)} {}
PageManager::~PageManager() = default;
void PageManager::OnGpuMap(VAddr address, size_t size) {
    impl->OnMap(address, size);
}
void PageManager::OnGpuUnmap(VAddr address, size_t size) {
    impl->OnUnmap(address, size);
}

template <bool track>
void PageManager::UpdatePageWatchers(VAddr addr, u64 size) const {
    impl->UpdatePageWatchers<track, false>(addr, size);
}

template <bool track, bool is_read>
void PageManager::UpdatePageWatchersForRegion(VAddr base_addr, RegionBits& mask) const {
    impl->UpdatePageWatchersForRegion<track, is_read>(base_addr, mask);
}

template void PageManager::UpdatePageWatchers<true>(VAddr addr, u64 size) const;
template void PageManager::UpdatePageWatchers<false>(VAddr addr, u64 size) const;
template void PageManager::UpdatePageWatchersForRegion<true, true>(VAddr base_addr,
                                                                   RegionBits& mask) const;
template void PageManager::UpdatePageWatchersForRegion<true, false>(VAddr base_addr,
                                                                    RegionBits& mask) const;
template void PageManager::UpdatePageWatchersForRegion<false, true>(VAddr base_addr,
                                                                    RegionBits& mask) const;
template void PageManager::UpdatePageWatchersForRegion<false, false>(VAddr base_addr,
                                                                     RegionBits& mask) const;

} // namespace VideoCore