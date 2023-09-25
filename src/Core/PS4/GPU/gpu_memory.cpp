#include "gpu_memory.h"

#include "Util/Singleton.h"

void GPU::memorySetAllocArea(u64 virtual_addr, u64 size) {
    auto* gpumemory = Singleton<GPUMemory>::Instance();

    Lib::LockMutexGuard lock(gpumemory->m_mutex);

    MemoryHeap h;
    h.allocated_virtual_addr = virtual_addr;
    h.allocated_size = size;

    gpumemory->m_heaps.push_back(h);
}
void* GPU::memoryCreateObj(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx, void* todo, u64 virtual_addr, u64 size, const GPUObject& info) {
    auto* gpumemory = Singleton<GPUMemory>::Instance();

    Lib::LockMutexGuard lock(gpumemory->m_mutex);

    int heap_id = gpumemory->getHeapId(virtual_addr, size);
    
    if (heap_id < 0)
    {
        return nullptr;
    }
    return nullptr;
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
