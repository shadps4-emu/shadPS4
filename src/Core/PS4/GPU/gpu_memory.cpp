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

    ObjInfo obj = {};

    // copy parameters from info to obj
    for (int i = 0; i < 8; i++) {
        obj.obj_params[i] = info.obj_params[i];
    }
    u64 hash[3] = {};  // assuming virtual_addr_num shouldn't be more that 3

    for (int h = 0; h < virtual_addr_num; h++) {
        if (info.hasHash) {
            hash[h] = GPU::calculate_hash(reinterpret_cast<const u08*>(virtual_addr[h]), size[h]);
        } else {
            hash[h] = 0;
        }
    }
    return nullptr;
}
