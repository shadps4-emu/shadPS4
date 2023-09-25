#pragma once

#include <types.h>
#include <Core/PS4/HLE/Graphics/graphics_ctx.h>

namespace GPU {
enum class MemoryMode : u32 { NoAccess = 0, Read = 1, Write = 2, ReadWrite = 3 };
enum class MemoryObjectType : u64 { InvalidObj, VideoOutBufferObj };
void MemorySetAllocArea(u64 virtual_addr, u64 size);

class GPUObject {
  public:
    GPUObject() = default;
    virtual ~GPUObject() = default;
};

void* memoryCreateObj(u64 submit_id, HLE::Libs::Graphics::GraphicCtx* ctx, /*CommandBuffer* buffer*/void* todo, u64 virtual_addr, u64 size,
                      const GPUObject& info);

}  // namespace GPU