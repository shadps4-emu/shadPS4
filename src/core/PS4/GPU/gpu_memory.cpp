// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <xxh3.h>
#include "common/singleton.h"
#include "core/PS4/GPU/gpu_memory.h"

void* GPU::memoryCreateObj(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx,
                           void* todo /*CommandBuffer?*/, u64 virtual_addr, u64 size,
                           const GPUObject& info) {
    auto* gpumemory = Common::Singleton<GPUMemory>::Instance();

    return gpumemory->memoryCreateObj(submit_id, ctx, nullptr, &virtual_addr, &size, 1, info);
}

void GPU::memorySetAllocArea(u64 virtual_addr, u64 size) {
    auto* gpumemory = Common::Singleton<GPUMemory>::Instance();

    std::scoped_lock lock{gpumemory->m_mutex};

    MemoryHeap h;
    h.allocated_virtual_addr = virtual_addr;
    h.allocated_size = size;

    gpumemory->m_heaps.push_back(h);
}

u64 GPU::calculate_hash(const u8* buf, u64 size) {
    return (size > 0 && buf != nullptr ? XXH3_64bits(buf, size) : 0);
}

bool GPU::vulkanAllocateMemory(HLE::Libs::Graphics::GraphicCtx* ctx,
                               HLE::Libs::Graphics::VulkanMemory* mem) {
    static std::atomic_uint64_t unique_id = 0;

    VkPhysicalDeviceMemoryProperties memory_properties{};
    vkGetPhysicalDeviceMemoryProperties(ctx->m_physical_device, &memory_properties);

    u32 index = 0;
    for (; index < memory_properties.memoryTypeCount; index++) {
        if ((mem->requirements.memoryTypeBits & (static_cast<uint32_t>(1) << index)) != 0 &&
            (memory_properties.memoryTypes[index].propertyFlags & mem->property) == mem->property) {
            break;
        }
    }

    mem->type = index;
    mem->offset = 0;

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.allocationSize = mem->requirements.size;
    alloc_info.memoryTypeIndex = index;

    mem->unique_id = ++unique_id;

    auto result = vkAllocateMemory(ctx->m_device, &alloc_info, nullptr, &mem->memory);

    if (result == VK_SUCCESS) {
        return true;
    }
    return false;
}

void GPU::flushGarlic(HLE::Libs::Graphics::GraphicCtx* ctx) {
    auto* gpumemory = Common::Singleton<GPUMemory>::Instance();
    gpumemory->flushAllHeaps(ctx);
}

int GPU::GPUMemory::getHeapId(u64 virtual_addr, u64 size) {
    int index = 0;
    for (const auto& heap : m_heaps) {
        if ((virtual_addr >= heap.allocated_virtual_addr &&
             virtual_addr < heap.allocated_virtual_addr + heap.allocated_size) ||
            ((virtual_addr + size - 1) >= heap.allocated_virtual_addr &&
             (virtual_addr + size - 1) < heap.allocated_virtual_addr + heap.allocated_size)) {
            return index;
        }
        index++;
    }
    return -1;
}

void* GPU::GPUMemory::memoryCreateObj(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx,
                                      void* todo, const u64* virtual_addr, const u64* size,
                                      int virtual_addr_num, const GPUObject& info) {
    auto* gpumemory = Common::Singleton<GPUMemory>::Instance();

    std::scoped_lock lock{gpumemory->m_mutex};

    int heap_id = gpumemory->getHeapId(virtual_addr[0], size[0]);

    if (heap_id < 0) {
        return nullptr;
    }
    auto& heap = m_heaps[heap_id];

    ObjInfo objInfo = {};

    // copy parameters from info to obj
    for (int i = 0; i < 8; i++) {
        objInfo.obj_params[i] = info.obj_params[i];
    }

    objInfo.gpu_object.objectType = info.objectType;
    objInfo.gpu_object.obj = nullptr;

    for (int h = 0; h < virtual_addr_num; h++) {
        if (info.check_hash) {
            objInfo.hash[h] =
                GPU::calculate_hash(reinterpret_cast<const u8*>(virtual_addr[h]), size[h]);
        } else {
            objInfo.hash[h] = 0;
        }
    }
    objInfo.submit_id = submit_id;
    objInfo.check_hash = info.check_hash;

    objInfo.gpu_object.obj = info.getCreateFunc()(ctx, objInfo.obj_params, virtual_addr, size,
                                                  virtual_addr_num, &objInfo.mem);

    objInfo.update_func = info.getUpdateFunc();
    int index = static_cast<int>(heap.objects.size());

    HeapObject hobj{};
    hobj.block = createHeapBlock(virtual_addr, size, virtual_addr_num, heap_id, index);
    hobj.info = objInfo;
    hobj.free = false;
    heap.objects.push_back(hobj);

    return objInfo.gpu_object.obj;
}

GPU::HeapBlock GPU::GPUMemory::createHeapBlock(const u64* virtual_addr, const u64* size,
                                               int virtual_addr_num, int heap_id, int obj_id) {
    auto& heap = m_heaps[heap_id];

    GPU::HeapBlock heapBlock{};
    heapBlock.virtual_addr_num = virtual_addr_num;
    for (int vi = 0; vi < virtual_addr_num; vi++) {
        heapBlock.virtual_addr[vi] = virtual_addr[vi];
        heapBlock.size[vi] = size[vi];
    }
    return heapBlock;
}

void GPU::GPUMemory::update(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx, int heap_id,
                            int obj_id) {
    auto& heap = m_heaps[heap_id];

    auto& heapObj = heap.objects[obj_id];
    auto& objInfo = heapObj.info;
    bool need_update = false;

    if (submit_id > objInfo.submit_id) {
        uint64_t hash[3] = {};

        for (int i = 0; i < heapObj.block.virtual_addr_num; i++) {
            if (objInfo.check_hash) {
                hash[i] = GPU::calculate_hash(
                    reinterpret_cast<const uint8_t*>(heapObj.block.virtual_addr[i]),
                    heapObj.block.size[i]);
            } else {
                hash[i] = 0;
            }
        }

        for (int i = 0; i < heapObj.block.virtual_addr_num; i++) {
            if (objInfo.hash[i] != hash[i]) {
                need_update = true;
                objInfo.hash[i] = hash[i];
            }
        }

        if (submit_id != UINT64_MAX) {
            objInfo.submit_id = submit_id;
        }
    }

    if (need_update) {
        objInfo.update_func(ctx, objInfo.obj_params, objInfo.gpu_object.obj,
                            heapObj.block.virtual_addr, heapObj.block.size,
                            heapObj.block.virtual_addr_num);
    }
}

void GPU::GPUMemory::flushAllHeaps(HLE::Libs::Graphics::GraphicCtx* ctx) {
    std::scoped_lock lock{m_mutex};

    int heap_id = 0;
    for (auto& heap : m_heaps) {
        int index = 0;
        for (auto& heapObj : heap.objects) {
            if (!heapObj.free) {
                update(UINT64_MAX, ctx, heap_id, index);
            }
            index++;
        }
        heap_id++;
    }
}
