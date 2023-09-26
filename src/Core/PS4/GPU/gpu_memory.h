#pragma once

#include <Core/PS4/HLE/Graphics/graphics_ctx.h>
#include <types.h>
#include <vector>

namespace GPU {

class GPUObject;

enum class MemoryMode : u32 { NoAccess = 0, Read = 1, Write = 2, ReadWrite = 3 };
enum class MemoryObjectType : u64 { InvalidObj, VideoOutBufferObj };

struct MemoryHeap {
    u64 allocated_virtual_addr = 0;
    u64 allocated_size = 0;
};

struct GpuMemoryObject {
    MemoryObjectType objectType = MemoryObjectType::InvalidObj;
    void* obj = nullptr;
};

struct ObjInfo {
    u64 obj_params[8] = {};
    GpuMemoryObject gpu_object;
    u64 hash[3] = {};
    u64 submit_id = 0;
    bool check_hash = false;
    HLE::Libs::Graphics::VulkanMemory mem;
};


class GPUMemory {
  public:
    GPUMemory() {}
    virtual ~GPUMemory() {}
    int getHeapId(u64 vaddr, u64 size);
    Lib::Mutex m_mutex;
    std::vector<MemoryHeap> m_heaps;
    void* memoryCreateObj(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx, /*CommandBuffer* buffer*/ void* todo, const u64* virtual_addr,
                          const u64* size, int virtual_addr_num, const GPUObject& info);
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

void memorySetAllocArea(u64 virtual_addr, u64 size);
void* memoryCreateObj(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx, /*CommandBuffer* buffer*/ void* todo, u64 virtual_addr, u64 size,
                      const GPUObject& info);
u64 calculate_hash(const u08* buf, u64 size);
bool vulkanAllocateMemory(HLE::Libs::Graphics::GraphicCtx* ctx, HLE::Libs::Graphics::VulkanMemory* mem);
}  // namespace GPU