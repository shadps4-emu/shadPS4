// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "video_core/buffer_cache/buffer.h"
#include "video_core/renderer_vulkan/liverpool_to_vk.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_platform.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"

#include <vk_mem_alloc.h>

namespace VideoCore {

std::string_view BufferTypeName(MemoryUsage type) {
    switch (type) {
    case MemoryUsage::Upload:
        return "Upload";
    case MemoryUsage::Download:
        return "Download";
    case MemoryUsage::Stream:
        return "Stream";
    case MemoryUsage::DeviceLocal:
        return "DeviceLocal";
    default:
        return "Invalid";
    }
}

[[nodiscard]] VkMemoryPropertyFlags MemoryUsagePreferredVmaFlags(MemoryUsage usage) {
    return usage != MemoryUsage::DeviceLocal ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                                             : VkMemoryPropertyFlagBits{};
}

[[nodiscard]] VmaAllocationCreateFlags MemoryUsageVmaFlags(MemoryUsage usage) {
    switch (usage) {
    case MemoryUsage::Upload:
    case MemoryUsage::Stream:
        return VMA_ALLOCATION_CREATE_MAPPED_BIT |
               VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    case MemoryUsage::Download:
        return VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    case MemoryUsage::DeviceLocal:
        return {};
    }
    return {};
}

[[nodiscard]] VmaMemoryUsage MemoryUsageVma(MemoryUsage usage) {
    switch (usage) {
    case MemoryUsage::DeviceLocal:
    case MemoryUsage::Stream:
        return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    case MemoryUsage::Upload:
    case MemoryUsage::Download:
        return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    }
    return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
}

UniqueBuffer::UniqueBuffer(vk::Device device_, VmaAllocator allocator_)
    : device{device_}, allocator{allocator_} {}

UniqueBuffer::~UniqueBuffer() {
    if (buffer) {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }
}

void UniqueBuffer::Create(const vk::BufferCreateInfo& buffer_ci, MemoryUsage usage,
                          VmaAllocationInfo* out_alloc_info) {
    const bool with_bda = bool(buffer_ci.usage & vk::BufferUsageFlagBits::eShaderDeviceAddress);
    const VmaAllocationCreateFlags bda_flag =
        with_bda ? VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT : 0;
    const VmaAllocationCreateInfo alloc_ci = {
        .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT | bda_flag | MemoryUsageVmaFlags(usage),
        .usage = MemoryUsageVma(usage),
        .requiredFlags = 0,
        .preferredFlags = MemoryUsagePreferredVmaFlags(usage),
        .pool = VK_NULL_HANDLE,
        .pUserData = nullptr,
    };

    const VkBufferCreateInfo buffer_ci_unsafe = static_cast<VkBufferCreateInfo>(buffer_ci);
    VkBuffer unsafe_buffer{};
    VkResult result = vmaCreateBuffer(allocator, &buffer_ci_unsafe, &alloc_ci, &unsafe_buffer,
                                      &allocation, out_alloc_info);
    ASSERT_MSG(result == VK_SUCCESS, "Failed allocating buffer with error {}",
               vk::to_string(vk::Result{result}));
    buffer = vk::Buffer{unsafe_buffer};

    if (with_bda) {
        vk::BufferDeviceAddressInfo bda_info{
            .buffer = buffer,
        };
        auto bda_result = device.getBufferAddress(bda_info);
        ASSERT_MSG(bda_result != 0, "Failed to get buffer device address");
        bda_addr = bda_result;
    }
}

Buffer::Buffer(const Vulkan::Instance& instance_, Vulkan::Scheduler& scheduler_, MemoryUsage usage_,
               VAddr cpu_addr_, vk::BufferUsageFlags flags, u64 size_bytes_)
    : cpu_addr{cpu_addr_}, size_bytes{size_bytes_}, instance{&instance_}, scheduler{&scheduler_},
      usage{usage_}, buffer{instance->GetDevice(), instance->GetAllocator()} {
    // Create buffer object.
    const vk::BufferCreateInfo buffer_ci = {
        .size = size_bytes,
        .usage = flags,
    };
    VmaAllocationInfo alloc_info{};
    buffer.Create(buffer_ci, usage, &alloc_info);

    const auto device = instance->GetDevice();
    Vulkan::SetObjectName(device, Handle(), "Buffer {:#x}:{:#x}", cpu_addr, size_bytes);

    // Map it if it is host visible.
    VkMemoryPropertyFlags property_flags{};
    vmaGetAllocationMemoryProperties(instance->GetAllocator(), buffer.allocation, &property_flags);
    if (alloc_info.pMappedData) {
        mapped_data = std::span<u8>{std::bit_cast<u8*>(alloc_info.pMappedData), size_bytes};
    }
    is_coherent = property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
}

ImportedHostBuffer::ImportedHostBuffer(const Vulkan::Instance& instance_,
                                       Vulkan::Scheduler& scheduler_, void* cpu_addr_,
                                       u64 size_bytes_, vk::BufferUsageFlags flags)
    : cpu_addr{cpu_addr_}, size_bytes{size_bytes_}, instance{&instance_}, scheduler{&scheduler_} {
    ASSERT_MSG(size_bytes > 0, "Size must be greater than 0");
    ASSERT_MSG(cpu_addr != 0, "CPU address must not be null");
    const vk::DeviceSize alignment = instance->GetExternalHostMemoryHostAlignment();
    ASSERT_MSG(reinterpret_cast<u64>(cpu_addr) % alignment == 0,
               "CPU address {:#x} is not aligned to {:#x}", cpu_addr, alignment);
    ASSERT_MSG(size_bytes % alignment == 0, "Size {:#x} is not aligned to {:#x}", size_bytes,
               alignment);

    // Test log, should be removed
    LOG_WARNING(Render_Vulkan, "Creating imported host buffer at {} size {:#x}", cpu_addr,
                size_bytes);

    const auto& mem_props = instance->GetMemoryProperties();
    auto ptr_props_result = instance->GetDevice().getMemoryHostPointerPropertiesEXT(
        vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT, cpu_addr);
    ASSERT_MSG(ptr_props_result.result == vk::Result::eSuccess,
               "Failed getting host pointer properties with error {}",
               vk::to_string(ptr_props_result.result));
    auto ptr_props = ptr_props_result.value;
    u32 memory_type_index = UINT32_MAX;
    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((ptr_props.memoryTypeBits & (1 << i)) != 0) {
            if (mem_props.memoryTypes[i].propertyFlags &
                (vk::MemoryPropertyFlagBits::eHostVisible |
                 vk::MemoryPropertyFlagBits::eHostCoherent)) {
                memory_type_index = i;
                // We prefer cache coherent memory types.
                if (mem_props.memoryTypes[i].propertyFlags &
                    vk::MemoryPropertyFlagBits::eHostCached) {
                    break;
                }
            }
        }
    }
    ASSERT_MSG(memory_type_index != UINT32_MAX,
               "Failed to find a host visible memory type for the imported host buffer");

    const bool with_bda = bool(flags & vk::BufferUsageFlagBits::eShaderDeviceAddress);
    vk::ExternalMemoryBufferCreateInfo external_info{
        .handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT,
    };
    vk::BufferCreateInfo buffer_ci{
        .pNext = &external_info,
        .size = size_bytes,
        .usage = flags,
    };
    vk::ImportMemoryHostPointerInfoEXT import_info{
        .handleType = vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT,
        .pHostPointer = reinterpret_cast<void*>(cpu_addr),
    };
    vk::MemoryAllocateFlagsInfo memory_flags_info{
        .pNext = &import_info,
        .flags = with_bda ? vk::MemoryAllocateFlagBits::eDeviceAddress : vk::MemoryAllocateFlags{},
    };
    vk::MemoryAllocateInfo alloc_ci{
        .pNext = &memory_flags_info,
        .allocationSize = size_bytes,
        .memoryTypeIndex = memory_type_index,
    };

    auto buffer_result = instance->GetDevice().createBuffer(buffer_ci);
    ASSERT_MSG(buffer_result.result == vk::Result::eSuccess,
               "Failed creating imported host buffer with error {}",
               vk::to_string(buffer_result.result));
    buffer = buffer_result.value;

    auto device_memory_result = instance->GetDevice().allocateMemory(alloc_ci);
    if (device_memory_result.result != vk::Result::eSuccess) {
        // May fail to import the host memory if it is backed by a file. (AMD on Linux)
        LOG_WARNING(Render_Vulkan, "Failed to import host memory at {} size {:#x}, Reason: {}",
                    cpu_addr, size_bytes, vk::to_string(device_memory_result.result));
        instance->GetDevice().destroyBuffer(buffer);
        buffer = VK_NULL_HANDLE;
        has_failed = true;
        return;
    }
    device_memory = device_memory_result.value;

    auto result = instance->GetDevice().bindBufferMemory(buffer, device_memory, 0);
    ASSERT_MSG(result == vk::Result::eSuccess, "Failed binding imported host buffer with error {}",
               vk::to_string(result));

    if (with_bda) {
        vk::BufferDeviceAddressInfo bda_info{
            .buffer = buffer,
        };
        bda_addr = instance->GetDevice().getBufferAddress(bda_info);
        ASSERT_MSG(bda_addr != 0, "Failed getting buffer device address");
    }
}

ImportedHostBuffer::~ImportedHostBuffer() {
    if (!buffer) {
        return;
    }
    const auto device = instance->GetDevice();
    device.destroyBuffer(buffer);
    device.freeMemory(device_memory);
}

constexpr u64 WATCHES_INITIAL_RESERVE = 0x4000;
constexpr u64 WATCHES_RESERVE_CHUNK = 0x1000;

StreamBuffer::StreamBuffer(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
                           MemoryUsage usage, u64 size_bytes)
    : Buffer{instance, scheduler, usage, 0, AllFlags, size_bytes} {
    ReserveWatches(current_watches, WATCHES_INITIAL_RESERVE);
    ReserveWatches(previous_watches, WATCHES_INITIAL_RESERVE);
    const auto device = instance.GetDevice();
    Vulkan::SetObjectName(device, Handle(), "StreamBuffer({}):{:#x}", BufferTypeName(usage),
                          size_bytes);
}

std::pair<u8*, u64> StreamBuffer::Map(u64 size, u64 alignment) {
    if (!is_coherent && usage == MemoryUsage::Stream) {
        size = Common::AlignUp(size, instance->NonCoherentAtomSize());
    }

    ASSERT(size <= this->size_bytes);
    mapped_size = size;

    if (alignment > 0) {
        offset = Common::AlignUp(offset, alignment);
    }

    if (offset + size > this->size_bytes) {
        // The buffer would overflow, save the amount of used watches and reset the state.
        invalidation_mark = current_watch_cursor;
        current_watch_cursor = 0;
        offset = 0;

        // Swap watches and reset waiting cursors.
        std::swap(previous_watches, current_watches);
        wait_cursor = 0;
        wait_bound = 0;
    }

    const u64 mapped_upper_bound = offset + size;
    WaitPendingOperations(mapped_upper_bound);
    return std::make_pair(mapped_data.data() + offset, offset);
}

void StreamBuffer::Commit() {
    if (!is_coherent) {
        if (usage == MemoryUsage::Download) {
            vmaInvalidateAllocation(instance->GetAllocator(), buffer.allocation, offset,
                                    mapped_size);
        } else {
            vmaFlushAllocation(instance->GetAllocator(), buffer.allocation, offset, mapped_size);
        }
    }

    offset += mapped_size;
    if (current_watch_cursor + 1 >= current_watches.size()) {
        // Ensure that there are enough watches.
        ReserveWatches(current_watches, WATCHES_RESERVE_CHUNK);
    }

    auto& watch = current_watches[current_watch_cursor++];
    watch.upper_bound = offset;
    watch.tick = scheduler->CurrentTick();
}

void StreamBuffer::ReserveWatches(std::vector<Watch>& watches, std::size_t grow_size) {
    watches.resize(watches.size() + grow_size);
}

void StreamBuffer::WaitPendingOperations(u64 requested_upper_bound) {
    if (!invalidation_mark) {
        return;
    }
    while (requested_upper_bound > wait_bound && wait_cursor < *invalidation_mark) {
        auto& watch = previous_watches[wait_cursor];
        wait_bound = watch.upper_bound;
        scheduler->Wait(watch.tick);
        ++wait_cursor;
    }
}

} // namespace VideoCore
