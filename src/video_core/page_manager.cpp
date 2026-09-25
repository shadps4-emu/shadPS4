// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <utility>
#include "common/adaptive_mutex.h"
#include "common/assert.h"
#include "common/debug.h"
#include "common/div_ceil.h"
#include "common/error.h"
#include "common/signal_context.h"
#include "core/emulator_settings.h"
#include "core/memory.h"
#include "core/signals.h"
#include "video_core/page_manager.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"

#ifndef _WIN64
#include <sys/mman.h>
#include "common/adaptive_mutex.h"
#else
#include <windows.h>
#endif

#ifdef __linux__
#include <thread>
#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#else
#include "common/spin_lock.h"
#endif

namespace VideoCore {

struct PageManager::Impl {
    struct PageState {
        u8 num_write_watchers;
        u8 num_read_watchers;

        Core::MemoryPermission WritePerm() const noexcept {
            return num_write_watchers == 0 ? Core::MemoryPermission::Write
                                           : Core::MemoryPermission::None;
        }

        Core::MemoryPermission ReadPerm() const noexcept {
            return num_read_watchers == 0 ? Core::MemoryPermission::Read
                                          : Core::MemoryPermission::None;
        }

        Core::MemoryPermission Perms() const noexcept {
            return ReadPerm() | WritePerm();
        }

        template <bool is_read>
        u8 GetPage() const {
            if constexpr (is_read) {
                return num_read_watchers;
            } else {
                return num_write_watchers;
            }
        }

        constexpr Core::MemoryPermission Update(PageOp write_op, bool update_write = true,
                                                PageOp read_op = PageOp::None,
                                                bool update_read = false) {
            if (update_read) {
                if (read_op == PageOp::Track) {
                    ASSERT_MSG(num_read_watchers == 0, "Too many watchers");
                } else if (read_op == PageOp::Untrack) {
                    ASSERT_MSG(num_read_watchers > 0, "Not enough watchers");
                }
                num_read_watchers += std::to_underlying(read_op);
            }
            if (update_write) {
                if (write_op == PageOp::Track) {
                    ASSERT_MSG(num_write_watchers < 255, "Too many watchers");
                } else if (write_op == PageOp::Untrack) {
                    ASSERT_MSG(num_write_watchers > 0, "Not enough watchers");
                }
                num_write_watchers += std::to_underlying(write_op);
            }
            return Perms();
        }
    };

    static constexpr size_t ADDRESS_BITS = 40;
    static constexpr size_t NUM_ADDRESS_PAGES = 1ULL << (40 - PM_PAGE_BITS);
    static constexpr size_t NUM_ADDRESS_LOCKS = NUM_ADDRESS_PAGES / NUM_REGION_PAGES;
    inline static Vulkan::Rasterizer* rasterizer;

    Impl() = default;
    virtual ~Impl() = default;

    virtual void OnMap(VAddr address, size_t size) {
        // No-op
        EnsurePages(address, address + size);
    }

    virtual void OnUnmap(VAddr address, size_t size) {
        // No-op
    }

    virtual void Protect(VAddr address, size_t size, Core::MemoryPermission perms) = 0;

    void EnsurePages(VAddr begin, VAddr end) {
        const size_t start_page = begin >> PM_PAGE_BITS;
        const size_t end_page = end >> PM_PAGE_BITS;
        cached_pages.reserve(start_page, end_page);
        locks.reserve(start_page, end_page);
    }

    void UpdatePageWatchers(VAddr addr, u64 size, PageOp write_op) {
        const u64 page_start = addr >> PM_PAGE_BITS;
        const u64 page_end = Common::DivCeil(addr + size, PM_PAGE_SIZE);

        Core::MemoryPermission perms{};
        u64 range_begin = page_start;
        u64 range_pages = 0;
        u64 potential_pages = 0;

        const auto release_pending = [&] {
            if (range_pages > 0) {
                Protect(range_begin << PM_PAGE_BITS, range_pages << PM_PAGE_BITS, perms);
                range_pages = 0;
                potential_pages = 0;
            }
        };

        // Iterate requested pages
        const u64 aligned_addr = page_start << PM_PAGE_BITS;
        const u64 aligned_end = page_end << PM_PAGE_BITS;
        if (!rasterizer->IsMapped(aligned_addr, aligned_end - aligned_addr)) {
            LOG_WARNING(Render,
                        "Tracking memory region {:#x} - {:#x} which is not fully GPU mapped.",
                        aligned_addr, aligned_end);
            EnsurePages(aligned_addr, aligned_end);
        }

        for (u64 page = page_start; page != page_end; ++page) {
            PageState* state = cached_pages.find(page);
            if (!state) {
                continue;
            }

            locks[page].lock();

            const auto old_perms = state->Perms();
            if (page == page_start) {
                perms = old_perms;
            }

            // Apply the change to the page state
            const auto new_perms = state->Update(write_op);
            if (new_perms != perms) [[unlikely]] {
                // If the protection changed add pending (un)protect action
                release_pending();
                perms = new_perms;
            } else if (range_pages != 0) {
                ++potential_pages;
            }

            // If the page must be (un)protected
            if (new_perms != old_perms) {
                if (range_pages == 0) {
                    // Start a new potential range
                    range_begin = page;
                    potential_pages = 1;
                }
                // Extend current range up to potential range
                range_pages = potential_pages;
            }
        }

        // Add pending (un)protect action
        release_pending();

        for (u64 page = page_start; page != page_end; ++page) {
            if (auto* lock = locks.find(page)) {
                lock->unlock();
            }
        }
    }

    void UpdatePageWatchersForRegion(VAddr base_addr, const Bounds& bounds,
                                     const RegionBits& write_mask, const RegionBits& read_mask,
                                     PageOp write_op, PageOp read_op) {
        const u64 base_page = base_addr >> PM_PAGE_BITS;
        const u64 page_start = bounds.start_word * PAGES_PER_WORD + bounds.start_page;
        const u64 page_end = bounds.end_word * PAGES_PER_WORD + bounds.end_page + 1;

        Core::MemoryPermission perms{};
        u64 range_begin = base_page + page_start;
        u64 range_pages = 0;
        u64 potential_pages = 0;

        const auto release_pending = [&] {
            if (range_pages > 0) {
                Protect(range_begin << PM_PAGE_BITS, range_pages << PM_PAGE_BITS, perms);
                range_pages = 0;
                potential_pages = 0;
            }
        };

        for (u64 page = page_start; page != page_end; ++page) {
            PageState* state = cached_pages.find(base_page + page);
            if (!state) {
                continue;
            }

            locks[base_page + page].lock();

            const auto old_perms = state->Perms();
            if (page == page_start) {
                perms = old_perms;
            }

            // Apply the change to the page state
            const bool update_write = write_op != PageOp::None && write_mask.GetPage(page);
            const bool update_read = read_op != PageOp::None && read_mask.GetPage(page);
            const auto new_perms = state->Update(write_op, update_write, read_op, update_read);

            if (new_perms != perms) [[unlikely]] {
                // If the protection changed add pending (un)protect action
                release_pending();
                perms = new_perms;
            } else if (range_pages != 0) {
                // If the protection did not change, extend the potential range
                ++potential_pages;
            }

            // If the page must be (un)protected
            if (new_perms != old_perms) {
                if (range_pages == 0) {
                    // Start a new potential range
                    range_begin = base_page + page;
                    potential_pages = 1;
                }
                // Extend current rango up to potential range
                range_pages = potential_pages;
            }
        }

        // Add pending (un)protect action
        release_pending();

        for (u64 page = page_start; page != page_end; ++page) {
            if (auto* lock = locks.find(base_page + page)) {
                lock->unlock();
            }
        }
    }

    struct PageTraits {
        using Entry = PageState;
        static constexpr size_t ADDRESS_SPACE_BITS = ADDRESS_BITS;
        static constexpr size_t L1_BITS = 16;
        static constexpr size_t PAGE_BITS = PM_PAGE_BITS;
        static constexpr bool NULL_CHECK = false;
    };
    MultiLevelPageTable<PageTraits> cached_pages;
    struct MutexTraits {
#ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
        using Entry = Common::AdaptiveMutex;
#else
        using Entry = std::mutex;
#endif
        static constexpr size_t ADDRESS_SPACE_BITS = ADDRESS_BITS;
        static constexpr size_t L1_BITS = 16;
        static constexpr size_t PAGE_BITS = PM_PAGE_BITS;
        static constexpr bool NULL_CHECK = false;
    };
    MultiLevelPageTable<MutexTraits> locks;
};

#ifdef __linux__
struct UffdImpl : public PageManager::Impl {
private:
    std::jthread ufd_thread;
    int uffd;

public:
    UffdImpl(Vulkan::Rasterizer* rasterizer_) : Impl() {
        rasterizer = rasterizer_;
        uffd = syscall(SYS_userfaultfd, O_CLOEXEC | O_NONBLOCK | UFFD_USER_MODE_ONLY);
        if (uffd == -1) {
            LOG_ERROR(Common_Memory,
                      "userfaultfd syscall failed: {}, falling back to signal implementation",
                      Common::GetLastErrorMsg());
            throw std::runtime_error("userfaultfd");
        }

        // Request uffdio features from kernel.
        uffdio_api api;
        api.api = UFFD_API;
        api.features = UFFD_FEATURE_THREAD_ID;
        const int ret = ioctl(uffd, UFFDIO_API, &api);
        if (ret != 0) {
            LOG_ERROR(Common_Memory,
                      "uffdio_api call failed: {}, falling back to signal implementation",
                      Common::GetLastErrorMsg());
            throw std::runtime_error("uffdio_api");
        }

        // Create uffd handler thread
        ufd_thread = std::jthread([&](std::stop_token token) { UffdHandler(token); });
    }

    ~UffdImpl() = default;

    void OnMap(VAddr address, size_t size) override {
        uffdio_register reg;
        reg.range.start = address;
        reg.range.len = size;
        reg.mode = UFFDIO_REGISTER_MODE_WP;
        const int ret = ioctl(uffd, UFFDIO_REGISTER, &reg);
        ASSERT_MSG(ret != -1, "Uffdio register failed with error: {}", Common::GetLastErrorMsg());
    }

    void OnUnmap(VAddr address, size_t size) override {
        uffdio_range range;
        range.start = address;
        range.len = size;
        const int ret = ioctl(uffd, UFFDIO_UNREGISTER, &range);
        ASSERT_MSG(ret != -1, "Uffdio unregister failed with error: {}", Common::GetLastErrorMsg());
    }

    void Protect(VAddr address, size_t size, Core::MemoryPermission perms) override {
        bool allow_write = True(perms & Core::MemoryPermission::Write);
        uffdio_writeprotect wp;
        wp.range.start = address;
        wp.range.len = size;
        wp.mode = allow_write ? UFFDIO_WRITEPROTECT_MODE_DONTWAKE : UFFDIO_WRITEPROTECT_MODE_WP;
        const int ret = ioctl(uffd, UFFDIO_WRITEPROTECT, &wp);
        ASSERT_MSG(ret != -1, "Uffdio writeprotect failed with error: {}",
                   Common::GetLastErrorMsg());
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
            if (readret == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                LOG_ERROR(Common_Memory, "Unexpected result of uffd read: {}",
                          Common::GetLastErrorMsg());
                break;
            }
            ASSERT_MSG(readret == sizeof(msg), "Unexpected short read, exiting");
            ASSERT(msg.arg.pagefault.flags & UFFD_PAGEFAULT_FLAG_WP);

            // Notify rasterizer about the fault.
            const VAddr addr = msg.arg.pagefault.address;
            const auto ptid = msg.arg.pagefault.feat.ptid;
            rasterizer->InvalidateMemory(addr, 1,
                                         ptid == rasterizer->GetGpuCommandProcessorThreadId());

            // Some calls to InvalidateMemory never reach the UFFDIO_WRITEPROTECT ioctl in
            // ::Protect, therefore we use MODE_DONTWAKE and wake the thread with UFFDIO_WAKE here
            uffdio_range wake;
            wake.start = msg.arg.pagefault.address;
            wake.len = PageManager::PM_PAGE_SIZE;
            const int ret = ioctl(uffd, UFFDIO_WAKE, &wake);
            ASSERT_MSG(ret != -1, "Waking thread {} failed with: {}", ptid,
                       Common::GetLastErrorMsg());
        }
    }
};
#endif // __linux__

struct SignalImpl : public PageManager::Impl {
    SignalImpl(Vulkan::Rasterizer* rasterizer_) : Impl() {
        rasterizer = rasterizer_;

        // Should be called first.
        constexpr auto priority = std::numeric_limits<u32>::min();
        Core::Signals::Instance()->RegisterAccessViolationHandler(GuestFaultSignalHandler,
                                                                  priority);
    }

    void Protect(VAddr address, size_t size, Core::MemoryPermission perms) override {
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
        return false;
    }
};

PageManager::PageManager(Vulkan::Rasterizer* rasterizer_) {
#ifdef __linux__
    if (EmulatorSettings.IsUserfaultfdTracking()) {
        try {
            impl = std::make_unique<UffdImpl>(rasterizer_);
            LOG_INFO(Config, "Memory tracking method: userfaultfd");
            return;
        } catch (const std::runtime_error& e) {
            // if uffd is unsupported, falls back to SignalImpl
        }
    }
    LOG_INFO(Config, "Memory tracking method: signals");
#endif
    impl = std::make_unique<SignalImpl>(rasterizer_);
}

PageManager::~PageManager() = default;

void PageManager::OnGpuMap(VAddr address, size_t size) {
    impl->OnMap(address, size);
}

void PageManager::OnGpuUnmap(VAddr address, size_t size) {
    impl->OnUnmap(address, size);
}

void PageManager::UpdatePageWatchers(VAddr addr, u64 size, PageOp write_op) const {
    impl->UpdatePageWatchers(addr, size, write_op);
}

void PageManager::UpdatePageWatchersForRegion(VAddr base_addr, const Bounds& bounds,
                                              const RegionBits& write_mask,
                                              const RegionBits& read_mask, PageOp write_op,
                                              PageOp read_op) const {
    impl->UpdatePageWatchersForRegion(base_addr, bounds, write_mask, read_mask, write_op, read_op);
}

} // namespace VideoCore
