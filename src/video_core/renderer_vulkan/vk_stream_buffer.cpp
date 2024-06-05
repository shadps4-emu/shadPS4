// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include "common/alignment.h"
#include "common/assert.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_stream_buffer.h"

namespace Vulkan {

namespace {

std::string_view BufferTypeName(BufferType type) {
    switch (type) {
    case BufferType::Upload:
        return "Upload";
    case BufferType::Download:
        return "Download";
    case BufferType::Stream:
        return "Stream";
    default:
        return "Invalid";
    }
}

vk::MemoryPropertyFlags MakePropertyFlags(BufferType type) {
    switch (type) {
    case BufferType::Upload:
        return vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    case BufferType::Download:
        return vk::MemoryPropertyFlagBits::eHostVisible |
               vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostCached;
    case BufferType::Stream:
        return vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible |
               vk::MemoryPropertyFlagBits::eHostCoherent;
    default:
        UNREACHABLE_MSG("Unknown buffer type {}", static_cast<u32>(type));
        return vk::MemoryPropertyFlagBits::eHostVisible;
    }
}

static std::optional<u32> FindMemoryType(const vk::PhysicalDeviceMemoryProperties& properties,
                                         vk::MemoryPropertyFlags wanted) {
    for (u32 i = 0; i < properties.memoryTypeCount; ++i) {
        const auto flags = properties.memoryTypes[i].propertyFlags;
        if ((flags & wanted) == wanted) {
            return i;
        }
    }
    return std::nullopt;
}

/// Get the preferred host visible memory type.
u32 GetMemoryType(const vk::PhysicalDeviceMemoryProperties& properties, BufferType type) {
    vk::MemoryPropertyFlags flags = MakePropertyFlags(type);
    std::optional preferred_type = FindMemoryType(properties, flags);

    constexpr std::array remove_flags = {
        vk::MemoryPropertyFlagBits::eHostCached,
        vk::MemoryPropertyFlagBits::eHostCoherent,
    };

    for (u32 i = 0; i < remove_flags.size() && !preferred_type; i++) {
        flags &= ~remove_flags[i];
        preferred_type = FindMemoryType(properties, flags);
    }
    ASSERT_MSG(preferred_type, "No suitable memory type found");
    return preferred_type.value();
}

constexpr u64 WATCHES_INITIAL_RESERVE = 0x4000;
constexpr u64 WATCHES_RESERVE_CHUNK = 0x1000;

} // Anonymous namespace

StreamBuffer::StreamBuffer(const Instance& instance_, Scheduler& scheduler_,
                           vk::BufferUsageFlags usage_, u64 size, BufferType type_)
    : instance{instance_}, scheduler{scheduler_}, device{instance.GetDevice()},
      stream_buffer_size{size}, usage{usage_}, type{type_} {
    CreateBuffers(size);
    ReserveWatches(current_watches, WATCHES_INITIAL_RESERVE);
    ReserveWatches(previous_watches, WATCHES_INITIAL_RESERVE);
}

StreamBuffer::~StreamBuffer() {
    device.unmapMemory(memory);
    device.destroyBuffer(buffer);
    device.freeMemory(memory);
}

std::tuple<u8*, u64, bool> StreamBuffer::Map(u64 size, u64 alignment) {
    if (!is_coherent && type == BufferType::Stream) {
        size = Common::AlignUp(size, instance.NonCoherentAtomSize());
    }

    ASSERT(size <= stream_buffer_size);
    mapped_size = size;

    if (alignment > 0) {
        offset = Common::AlignUp(offset, alignment);
    }

    bool invalidate{false};
    if (offset + size > stream_buffer_size) {
        // The buffer would overflow, save the amount of used watches and reset the state.
        invalidate = true;
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

    return std::make_tuple(mapped + offset, offset, invalidate);
}

void StreamBuffer::Commit(u64 size) {
    if (!is_coherent && type == BufferType::Stream) {
        size = Common::AlignUp(size, instance.NonCoherentAtomSize());
    }

    ASSERT_MSG(size <= mapped_size, "Reserved size {} is too small compared to {}", mapped_size,
               size);

    const vk::MappedMemoryRange range = {
        .memory = memory,
        .offset = offset,
        .size = size,
    };

    if (!is_coherent && type == BufferType::Download) {
        device.invalidateMappedMemoryRanges(range);
    } else if (!is_coherent) {
        device.flushMappedMemoryRanges(range);
    }

    offset += size;

    if (current_watch_cursor + 1 >= current_watches.size()) {
        // Ensure that there are enough watches.
        ReserveWatches(current_watches, WATCHES_RESERVE_CHUNK);
    }
    auto& watch = current_watches[current_watch_cursor++];
    watch.upper_bound = offset;
    watch.tick = scheduler.CurrentTick();
}

void StreamBuffer::CreateBuffers(u64 prefered_size) {
    const vk::Device device = instance.GetDevice();
    const auto memory_properties = instance.GetPhysicalDevice().getMemoryProperties();
    const u32 preferred_type = GetMemoryType(memory_properties, type);
    const vk::MemoryType mem_type = memory_properties.memoryTypes[preferred_type];
    const u32 preferred_heap = mem_type.heapIndex;
    is_coherent =
        static_cast<bool>(mem_type.propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent);

    // Substract from the preferred heap size some bytes to avoid getting out of memory.
    const vk::DeviceSize heap_size = memory_properties.memoryHeaps[preferred_heap].size;
    // As per DXVK's example, using `heap_size / 2`
    const vk::DeviceSize allocable_size = heap_size / 2;
    buffer = device.createBuffer({
        .size = std::min(prefered_size, allocable_size),
        .usage = usage,
    });

    const auto requirements_chain =
        device
            .getBufferMemoryRequirements2<vk::MemoryRequirements2, vk::MemoryDedicatedRequirements>(
                {.buffer = buffer});

    const auto& requirements = requirements_chain.get<vk::MemoryRequirements2>();
    const auto& dedicated_requirements = requirements_chain.get<vk::MemoryDedicatedRequirements>();

    stream_buffer_size = static_cast<u64>(requirements.memoryRequirements.size);

    LOG_INFO(Render_Vulkan, "Creating {} buffer with size {} KiB with flags {}",
             BufferTypeName(type), stream_buffer_size / 1024,
             vk::to_string(mem_type.propertyFlags));

    if (dedicated_requirements.prefersDedicatedAllocation) {
        vk::StructureChain<vk::MemoryAllocateInfo, vk::MemoryDedicatedAllocateInfo> alloc_chain =
            {};

        auto& alloc_info = alloc_chain.get<vk::MemoryAllocateInfo>();
        alloc_info.allocationSize = requirements.memoryRequirements.size;
        alloc_info.memoryTypeIndex = preferred_type;

        auto& dedicated_alloc_info = alloc_chain.get<vk::MemoryDedicatedAllocateInfo>();
        dedicated_alloc_info.buffer = buffer;

        memory = device.allocateMemory(alloc_chain.get());
    } else {
        memory = device.allocateMemory({
            .allocationSize = requirements.memoryRequirements.size,
            .memoryTypeIndex = preferred_type,
        });
    }

    device.bindBufferMemory(buffer, memory, 0);
    mapped = reinterpret_cast<u8*>(device.mapMemory(memory, 0, VK_WHOLE_SIZE));

    if (instance.HasDebuggingToolAttached()) {
        SetObjectName(device, buffer, "StreamBuffer({}): {} KiB {}", BufferTypeName(type),
                      stream_buffer_size / 1024, vk::to_string(mem_type.propertyFlags));
        SetObjectName(device, memory, "StreamBufferMemory({}): {} Kib {}", BufferTypeName(type),
                      stream_buffer_size / 1024, vk::to_string(mem_type.propertyFlags));
    }
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
        scheduler.Wait(watch.tick);
        ++wait_cursor;
    }
}

} // namespace Vulkan
