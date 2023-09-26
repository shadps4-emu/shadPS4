#include "gpu_memory.h"

#include <xxhash/xxh3.h>

#include "Util/Singleton.h"

void* GPU::memoryCreateObj(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx, void* todo /*CommandBuffer?*/, u64 virtual_addr, u64 size,
                           const GPUObject& info) {
    auto* gpumemory = Singleton<GPUMemory>::Instance();

    return gpumemory->memoryCreateObj(submit_id, ctx, nullptr, &virtual_addr, &size, 1, info);
}

void GPU::memorySetAllocArea(u64 virtual_addr, u64 size) {
    auto* gpumemory = Singleton<GPUMemory>::Instance();

    Lib::LockMutexGuard lock(gpumemory->m_mutex);

    MemoryHeap h;
    h.allocated_virtual_addr = virtual_addr;
    h.allocated_size = size;

    gpumemory->m_heaps.push_back(h);
}

u64 GPU::calculate_hash(const u08* buf, u64 size) { return (size > 0 && buf != nullptr ? XXH3_64bits(buf, size) : 0); }

bool GPU::vulkanAllocateMemory(HLE::Libs::Graphics::GraphicCtx* ctx, HLE::Libs::Graphics::VulkanMemory* mem) {
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

int GPU::GPUMemory::getHeapId(u64 virtual_addr, u64 size) {
    int index = 0;
    for (const auto& heap : m_heaps) {
        if ((virtual_addr >= heap.allocated_virtual_addr && virtual_addr < heap.allocated_virtual_addr + heap.allocated_size) ||
            ((virtual_addr + size - 1) >= heap.allocated_virtual_addr &&
             (virtual_addr + size - 1) < heap.allocated_virtual_addr + heap.allocated_size)) {
            return index;
        }
        index++;
    }
    return -1;
}

void* GPU::GPUMemory::memoryCreateObj(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx, void* todo, const u64* virtual_addr, const u64* size,
                                      int virtual_addr_num, const GPUObject& info) {
    auto* gpumemory = Singleton<GPUMemory>::Instance();

    Lib::LockMutexGuard lock(gpumemory->m_mutex);

    int heap_id = gpumemory->getHeapId(virtual_addr[0], size[0]);

    if (heap_id < 0) {
        return nullptr;
    }

    ObjInfo objInfo = {};

    // copy parameters from info to obj
    for (int i = 0; i < 8; i++) {
        objInfo.obj_params[i] = info.obj_params[i];
    }

    objInfo.gpu_object.objectType = info.objectType;
    objInfo.gpu_object.obj = nullptr;

    for (int h = 0; h < virtual_addr_num; h++) {
        if (info.check_hash) {
            objInfo.hash[h] = GPU::calculate_hash(reinterpret_cast<const u08*>(virtual_addr[h]), size[h]);
        } else {
            objInfo.hash[h] = 0;
        }
    }
    objInfo.submit_id = submit_id;
    objInfo.check_hash = info.check_hash;

    objInfo.gpu_object.obj = info.getCreateFunc()(ctx, objInfo.obj_params, virtual_addr, size, virtual_addr_num, &objInfo.mem);

    // TODO we need more ...
    return nullptr;
}
