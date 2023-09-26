#include "gpu_memory.h"

#include "Util/Singleton.h"

void* GPU::memoryCreateObj(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx, void* todo /*CommandBuffer?*/, u64 virtual_addr, u64 size, const GPUObject& info) {
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
    // TODO not finished!
    return nullptr;
}
