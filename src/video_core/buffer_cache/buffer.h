// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "common/assert.h"
#include "common/types.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Vulkan {
class Instance;
class Scheduler;
} // namespace Vulkan

VK_DEFINE_HANDLE(VmaAllocation)
VK_DEFINE_HANDLE(VmaAllocator)

struct VmaAllocationInfo;

namespace VideoCore {

/// Hints and requirements for the backing memory type of a commit
enum class MemoryType : u8 {
    DeviceLocal,  ///< Requests device local buffer.
    HostUncached, ///< Requires a host visible memory type optimized for CPU to GPU uploads
    HostCached,   ///< Requires a host visible memory type optimized for GPU to CPU readbacks
    Stream,       ///< Requests device local host visible buffer, falling back host memory.
    Sparse,       ///< Requires an unbacked sparse resident buffer.
};

constexpr vk::BufferUsageFlags ReadFlags =
    vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eUniformBuffer |
    vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
    vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;

constexpr vk::BufferUsageFlags AllFlags =
    ReadFlags | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer;

struct UniqueBuffer {
    explicit UniqueBuffer(vk::Device device, VmaAllocator allocator);
    ~UniqueBuffer();

    UniqueBuffer(const UniqueBuffer&) = delete;
    UniqueBuffer& operator=(const UniqueBuffer&) = delete;

    UniqueBuffer(UniqueBuffer&& other) noexcept
        : device{other.device}, buffer{std::exchange(other.buffer, VK_NULL_HANDLE)},
          bda_addr{std::exchange(other.bda_addr, 0)},
          allocator{std::exchange(other.allocator, VK_NULL_HANDLE)},
          allocation{std::exchange(other.allocation, VK_NULL_HANDLE)} {}

    UniqueBuffer& operator=(UniqueBuffer&& other) noexcept {
        if (this != &other) {
            Destroy();
            device = other.device;
            buffer = std::exchange(other.buffer, VK_NULL_HANDLE);
            bda_addr = std::exchange(other.bda_addr, 0);
            allocator = std::exchange(other.allocator, VK_NULL_HANDLE);
            allocation = std::exchange(other.allocation, VK_NULL_HANDLE);
        }
        return *this;
    }

    void Create(vk::BufferCreateInfo& buffer_ci, MemoryType mem_type,
                VmaAllocationInfo* out_alloc_info);

    void Destroy();

    vk::Device device;
    vk::Buffer buffer{};
    vk::DeviceAddress bda_addr{};
    VmaAllocator allocator;
    VmaAllocation allocation{};
};

struct Buffer {
    explicit Buffer(const Vulkan::Instance& instance, VAddr cpu_addr_, u64 size_bytes_,
                    MemoryType mem_type, std::string_view debug_name = "");

    Buffer& operator=(const Buffer&) = delete;
    Buffer(const Buffer&) = delete;

    Buffer& operator=(Buffer&&) = default;
    Buffer(Buffer&&) = default;

    [[nodiscard]] bool IsInBounds(VAddr addr, u64 size) const noexcept {
        return addr >= cpu_addr && addr + size <= cpu_addr + SizeBytes();
    }

    [[nodiscard]] VAddr CpuAddr() const noexcept {
        return cpu_addr;
    }

    [[nodiscard]] u64 Offset(VAddr other_cpu_addr) const noexcept {
        return other_cpu_addr - cpu_addr;
    }

    size_t SizeBytes() const {
        return size_bytes;
    }

    vk::Buffer Handle() const noexcept {
        return buffer.buffer;
    }

    vk::DeviceAddress BufferDeviceAddress() const noexcept {
        ASSERT_MSG(buffer.bda_addr != 0, "Can't get BDA from a non BDA buffer");
        return buffer.bda_addr;
    }

    /// Makes host writes in provided range visible to the device.
    void Flush(u64 offset, u64 size);

    /// Makes device writes in provided range visible to the host.
    void Invalidate(u64 offset, u64 size);

    VAddr cpu_addr = 0;
    size_t size_bytes = 0;
    std::span<u8> mapped_data{};
    bool is_coherent{};
    MemoryType mem_type{MemoryType::DeviceLocal};
    UniqueBuffer buffer;
};

struct StreamBuffer : public Buffer {
    explicit StreamBuffer(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
                          MemoryType mem_type, u64 size_bytes);

    /// Reserves a region of memory from the stream buffer. Must be followed by Commit().
    std::pair<u8*, u64> Map(u64 size, u64 alignment = 0, bool allow_wait = true);

    /// Ensures that reserved bytes of memory are available to the GPU.
    void Commit();

    /// Reserves and commits a region in one step.
    std::optional<u64> Reserve(u64 size, u64 alignment = 0, bool allow_wait = true);

    /// Tick of the most recent commit into this buffer, 0 if never used.
    [[nodiscard]] u64 LastTick() const noexcept {
        return last_tick;
    }

    /// Maps and commits a memory region with user provided data
    u64 Copy(auto src, size_t size, size_t alignment = 0) {
        const auto [data, offset] = Map(size, alignment);
        ASSERT_MSG(data, "Failed to copy {:#x} bytes", size);
        std::memcpy(data, reinterpret_cast<const void*>(src), size);
        Commit();
        return offset;
    }

private:
    struct Watch {
        u64 tick{};
        u64 upper_bound{};
    };

    /// Aligns, wraps if needed and waits for the GPU. On success mapped_size is set and the
    /// region starts at offset.
    bool PrepareMap(u64 size, u64 alignment, bool allow_wait);

    /// Advances offset past the mapped region and records the current tick for it.
    void AdvanceAndWatch();

    /// Increases the amount of watches available.
    void ReserveWatches(std::vector<Watch>& watches, std::size_t grow_size);

    /// Waits pending watches until requested upper bound.
    bool WaitPendingOperations(u64 requested_upper_bound, bool allow_wait);

private:
    Vulkan::Scheduler& scheduler;
    vk::DeviceSize non_coherent_atom_size{};
    u64 offset{};
    u64 mapped_size{};
    u64 last_tick{};
    std::vector<Watch> current_watches;
    std::size_t current_watch_cursor{};
    std::optional<size_t> invalidation_mark;
    std::vector<Watch> previous_watches;
    std::size_t wait_cursor{};
    u64 wait_bound{};
};

} // namespace VideoCore
