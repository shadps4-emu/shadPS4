// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>
#include "common/types.h"
#include "video_core/amdgpu/resource.h"
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
enum class MemoryUsage {
    DeviceLocal, ///< Requests device local buffer.
    Upload,      ///< Requires a host visible memory type optimized for CPU to GPU uploads
    Download,    ///< Requires a host visible memory type optimized for GPU to CPU readbacks
    Stream,      ///< Requests device local host visible buffer, falling back host memory.
};

struct UniqueBuffer {
    explicit UniqueBuffer(vk::Device device, VmaAllocator allocator);
    ~UniqueBuffer();

    UniqueBuffer(const UniqueBuffer&) = delete;
    UniqueBuffer& operator=(const UniqueBuffer&) = delete;

    UniqueBuffer(UniqueBuffer&& other)
        : allocator{std::exchange(other.allocator, VK_NULL_HANDLE)},
          allocation{std::exchange(other.allocation, VK_NULL_HANDLE)},
          buffer{std::exchange(other.buffer, VK_NULL_HANDLE)} {}
    UniqueBuffer& operator=(UniqueBuffer&& other) {
        buffer = std::exchange(other.buffer, VK_NULL_HANDLE);
        allocator = std::exchange(other.allocator, VK_NULL_HANDLE);
        allocation = std::exchange(other.allocation, VK_NULL_HANDLE);
        return *this;
    }

    void Create(const vk::BufferCreateInfo& image_ci, MemoryUsage usage,
                VmaAllocationInfo* out_alloc_info);

    operator vk::Buffer() const {
        return buffer;
    }

    vk::Device device;
    VmaAllocator allocator;
    VmaAllocation allocation;
    vk::Buffer buffer{};
};

class Buffer {
public:
    explicit Buffer(const Vulkan::Instance& instance, MemoryUsage usage, VAddr cpu_addr_,
                    u64 size_bytes_);

    Buffer& operator=(const Buffer&) = delete;
    Buffer(const Buffer&) = delete;

    Buffer& operator=(Buffer&&) = default;
    Buffer(Buffer&&) = default;

    vk::BufferView View(u32 offset, u32 size, AmdGpu::DataFormat dfmt, AmdGpu::NumberFormat nfmt);

    /// Increases the likeliness of this being a stream buffer
    void IncreaseStreamScore(int score) noexcept {
        stream_score += score;
    }

    /// Returns the likeliness of this being a stream buffer
    [[nodiscard]] int StreamScore() const noexcept {
        return stream_score;
    }

    /// Returns true when vaddr -> vaddr+size is fully contained in the buffer
    [[nodiscard]] bool IsInBounds(VAddr addr, u64 size) const noexcept {
        return addr >= cpu_addr && addr + size <= cpu_addr + SizeBytes();
    }

    /// Returns the base CPU address of the buffer
    [[nodiscard]] VAddr CpuAddr() const noexcept {
        return cpu_addr;
    }

    /// Returns the offset relative to the given CPU address
    [[nodiscard]] u32 Offset(VAddr other_cpu_addr) const noexcept {
        return static_cast<u32>(other_cpu_addr - cpu_addr);
    }

    size_t SizeBytes() const {
        return size_bytes;
    }

    vk::Buffer Handle() const noexcept {
        return buffer;
    }

public:
    VAddr cpu_addr = 0;
    bool is_picked{};
    bool is_coherent{};
    int stream_score = 0;
    size_t size_bytes = 0;
    std::span<u8> mapped_data;
    const Vulkan::Instance* instance{};
    MemoryUsage usage;
    UniqueBuffer buffer;
    struct BufferView {
        u32 offset;
        u32 size;
        AmdGpu::DataFormat dfmt;
        AmdGpu::NumberFormat nfmt;
        vk::BufferView handle;
    };
    std::vector<BufferView> views;
};

class StreamBuffer : public Buffer {
public:
    explicit StreamBuffer(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
                          MemoryUsage usage, u64 size_bytes_);

    /// Reserves a region of memory from the stream buffer.
    std::pair<u8*, u64> Map(u64 size, u64 alignment = 0);

    /// Ensures that reserved bytes of memory are available to the GPU.
    void Commit();

    /// Maps and commits a memory region with user provided data
    u64 Copy(VAddr src, size_t size, size_t alignment = 0) {
        const auto [data, offset] = Map(size, alignment);
        std::memcpy(data, reinterpret_cast<const void*>(src), size);
        Commit();
        return offset;
    }

    u64 GetFreeSize() const {
        return size_bytes - offset - mapped_size;
    }

private:
    struct Watch {
        u64 tick{};
        u64 upper_bound{};
    };

    /// Increases the amount of watches available.
    void ReserveWatches(std::vector<Watch>& watches, std::size_t grow_size);

    /// Waits pending watches until requested upper bound.
    void WaitPendingOperations(u64 requested_upper_bound);

private:
    Vulkan::Scheduler& scheduler;
    u64 offset{};
    u64 mapped_size{};
    std::vector<Watch> current_watches;
    std::size_t current_watch_cursor{};
    std::optional<size_t> invalidation_mark;
    std::vector<Watch> previous_watches;
    std::size_t wait_cursor{};
    u64 wait_bound{};
};

} // namespace VideoCore
