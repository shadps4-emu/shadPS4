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
#ifdef ENABLE_USERFAULTFD
#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <poll.h>
#include <sys/ioctl.h>
#endif
#else
#include <windows.h>
#endif

namespace VideoCore {

constexpr size_t PAGESIZE = 4_KB;
constexpr size_t PAGEBITS = 12;

#if ENABLE_USERFAULTFD
struct PageManager::Impl {
    Impl(Vulkan::Rasterizer* rasterizer_) : rasterizer{rasterizer_} {
        uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
        ASSERT_MSG(uffd != -1, "{}", Common::GetLastErrorMsg());

        // Request uffdio features from kernel.
        uffdio_api api;
        api.api = UFFD_API;
        api.features = UFFD_FEATURE_THREAD_ID;
        const int ret = ioctl(uffd, UFFDIO_API, &api);
        ASSERT(ret == 0 && api.api == UFFD_API);

        // Create uffd handler thread
        ufd_thread = std::jthread([&](std::stop_token token) { UffdHandler(token); });
    }

    void OnMap(VAddr address, size_t size) {
        uffdio_register reg;
        reg.range.start = address;
        reg.range.len = size;
        reg.mode = UFFDIO_REGISTER_MODE_WP;
        const int ret = ioctl(uffd, UFFDIO_REGISTER, &reg);
        ASSERT_MSG(ret != -1, "Uffdio register failed");
    }

    void OnUnmap(VAddr address, size_t size) {
        uffdio_range range;
        range.start = address;
        range.len = size;
        const int ret = ioctl(uffd, UFFDIO_UNREGISTER, &range);
        ASSERT_MSG(ret != -1, "Uffdio unregister failed");
    }

    void Protect(VAddr address, size_t size, bool allow_write) {
        uffdio_writeprotect wp;
        wp.range.start = address;
        wp.range.len = size;
        wp.mode = allow_write ? 0 : UFFDIO_WRITEPROTECT_MODE_WP;
        const int ret = ioctl(uffd, UFFDIO_WRITEPROTECT, &wp);
        ASSERT_MSG(ret != -1, "Uffdio writeprotect failed with error: {}",
                   Common::GetLastErrorMsg());
    }

    void UffdHandler(std::stop_token token) {
        while (!token.stop_requested()) {
            pollfd pollfd;
            pollfd.fd = uffd;
            pollfd.events = POLLIN;

            // Block until the descriptor is ready for data reads.
            const int pollres = poll(&pollfd, 1, -1);
            switch (pollres) {
            case -1:
                perror("Poll userfaultfd");
                continue;
                break;
            case 0:
                continue;
            case 1:
                break;
            default:
                UNREACHABLE_MSG("Unexpected number of descriptors {} out of poll", pollres);
            }

            // We don't want an error condition to have occured.
            ASSERT_MSG(!(pollfd.revents & POLLERR), "POLLERR on userfaultfd");

            // We waited until there is data to read, we don't care about anything else.
            if (!(pollfd.revents & POLLIN)) {
                continue;
            }

            // Read message from kernel.
            uffd_msg msg;
            const int readret = read(uffd, &msg, sizeof(msg));
            ASSERT_MSG(readret != -1 || errno == EAGAIN, "Unexpected result of uffd read");
            if (errno == EAGAIN) {
                continue;
            }
            ASSERT_MSG(readret == sizeof(msg), "Unexpected short read, exiting");
            ASSERT(msg.arg.pagefault.flags & UFFD_PAGEFAULT_FLAG_WP);

            // Notify rasterizer about the fault.
            const VAddr addr = msg.arg.pagefault.address;
            const VAddr addr_page = Common::AlignDown(addr, PAGESIZE);
            rasterizer->InvalidateMemory(addr_page, PAGESIZE);
        }
    }

    Vulkan::Rasterizer* rasterizer;
    std::jthread ufd_thread;
    int uffd;
};
#else
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
#endif

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
