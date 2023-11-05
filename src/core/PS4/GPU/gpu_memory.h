#pragma once

#include <core/PS4/HLE/Graphics/graphics_ctx.h>
#include "common/types.h"
#include <mutex>
#include <vector>

namespace GPU {

class GPUObject;

enum class MemoryMode : u32 { NoAccess = 0, Read = 1, Write = 2, ReadWrite = 3 };
enum class MemoryObjectType : u64 { InvalidObj, VideoOutBufferObj };

struct GpuMemoryObject {
    MemoryObjectType objectType = MemoryObjectType::InvalidObj;
    void* obj = nullptr;
};

struct HeapBlock {
    u64 virtual_addr[3] = {};
    u64 size[3] = {};
    int virtual_addr_num = 0;
};

class GPUObject {
  public:
    GPUObject() = default;
    virtual ~GPUObject() = default;
    u64 obj_params[8] = {};
    bool check_hash = false;
    bool isReadOnly = false;
    MemoryObjectType objectType = MemoryObjectType::InvalidObj;

    using create_func_t = void* (*)(HLE::Libs::Graphics::GraphicCtx* ctx, const u64* params, const u64* virtual_addr, const u64* size, int virtual_addr_num,
                                    HLE::Libs::Graphics::VulkanMemory* mem);
    using update_func_t = void (*)(HLE::Libs::Graphics::GraphicCtx* ctx, const u64* params, void* obj, const u64* virtual_addr, const u64* size,
                                   int virtual_addr_num);

    virtual create_func_t getCreateFunc() const = 0;
    virtual update_func_t getUpdateFunc() const = 0;
};

struct ObjInfo {
    u64 obj_params[8] = {};
    GpuMemoryObject gpu_object;
    u64 hash[3] = {};
    u64 submit_id = 0;
    bool check_hash = false;
    HLE::Libs::Graphics::VulkanMemory mem;
    GPU::GPUObject::update_func_t update_func = nullptr;
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
    std::mutex m_mutex;
    std::vector<MemoryHeap> m_heaps;
    void* memoryCreateObj(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx, /*CommandBuffer* buffer*/ void* todo, const u64* virtual_addr,
                          const u64* size, int virtual_addr_num, const GPUObject& info);
    HeapBlock createHeapBlock(const u64* virtual_addr, const u64* size, int virtual_addr_num, int heap_id, int obj_id);
    void update(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx, int heap_id, int obj_id);
    void flushAllHeaps(HLE::Libs::Graphics::GraphicCtx* ctx);
};

void memorySetAllocArea(u64 virtual_addr, u64 size);
void* memoryCreateObj(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx, /*CommandBuffer* buffer*/ void* todo, u64 virtual_addr, u64 size,
                      const GPUObject& info);
u64 calculate_hash(const u08* buf, u64 size);
bool vulkanAllocateMemory(HLE::Libs::Graphics::GraphicCtx* ctx, HLE::Libs::Graphics::VulkanMemory* mem);
void flushGarlic(HLE::Libs::Graphics::GraphicCtx* ctx);
}  // namespace GPU