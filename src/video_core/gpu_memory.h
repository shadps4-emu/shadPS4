#pragma once

#include <mutex>
#include <vector>
#include "common/types.h"

namespace VideoCore {

class GPUObject;

enum class MemoryMode : u32 {
    NoAccess = 0,
    Read = 1,
    Write = 2,
    ReadWrite = 3,
};

enum class MemoryObjectType : u64 {
    Invalid,
    VideoOutBuffer,
};

struct GpuMemoryObject {
    MemoryObjectType object_type = MemoryObjectType::Invalid;
    void* obj = nullptr;
};

struct HeapBlock {
    std::array<u64, 3> virtual_address{};
    std::array<u64, 3> size{};
    u32 virtual_addr_num = 0;
};

class GPUObject {
public:
    GPUObject() = default;
    virtual ~GPUObject() = default;
    u64 obj_params[8] = {};
    bool check_hash = false;
    bool isReadOnly = false;
    MemoryObjectType objectType = MemoryObjectType::Invalid;
};

struct ObjInfo {
    std::array<u64, 8> obj_params{};
    GpuMemoryObject gpu_object;
    std::array<u64, 3> hash{};
    u64 submit_id = 0;
    bool check_hash = false;
};

struct HeapObject {
    HeapBlock block;
    ObjInfo info;
    bool free = true;
};
struct MemoryHeap {
    u64 allocated_virtual_addr = 0;
    u64 allocated_size = 0;
    std::vector<HeapObject> objects;
};

class GPUMemory {
public:
    GPUMemory() {}
    virtual ~GPUMemory() {}
    int getHeapId(u64 vaddr, u64 size);
    void* memoryCreateObj(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx,
                          /*CommandBuffer* buffer*/ void* todo, const u64* virtual_addr,
                          const u64* size, int virtual_addr_num, const GPUObject& info);
    HeapBlock createHeapBlock(const u64* virtual_addr, const u64* size, int virtual_addr_num,
                              int heap_id, int obj_id);
    void update(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx, int heap_id, int obj_id);
    void flushAllHeaps(HLE::Libs::Graphics::GraphicCtx* ctx);

private:
    std::mutex m_mutex;
    std::vector<MemoryHeap> m_heaps;
};

void memorySetAllocArea(u64 virtual_addr, u64 size);
void* memoryCreateObj(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx,
                      /*CommandBuffer* buffer*/ void* todo, u64 virtual_addr, u64 size,
                      const GPUObject& info);
u64 calculate_hash(const u08* buf, u64 size);
bool vulkanAllocateMemory(HLE::Libs::Graphics::GraphicCtx* ctx,
                          HLE::Libs::Graphics::VulkanMemory* mem);
void flushGarlic(HLE::Libs::Graphics::GraphicCtx* ctx);

} // namespace VideoCore
