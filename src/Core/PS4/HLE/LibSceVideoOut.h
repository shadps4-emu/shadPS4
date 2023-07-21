#pragma once
#include "../Loader/SymbolsResolver.h"

namespace HLE::Libs::LibSceVideoOut {

typedef int32_t SceUserServiceUserId; //TODO move it to proper place

void LibSceVideoOut_Register(SymbolsResolver* sym);
//functions
int32_t sceVideoOutGetFlipStatus(int32_t handle /*, SceVideoOutFlipStatus* status*/);
int32_t sceVideoOutSubmitFlip(int32_t handle, int32_t bufferIndex, int32_t flipMode, int64_t flipArg);
int32_t sceVideoOutRegisterBuffers(int32_t handle, int32_t startIndex, void* const* addresses, int32_t bufferNum /*,const SceVideoOutBufferAttribute* attribute*/);
int32_t sceVideoOutAddFlipEvent(/*SceKernelEqueue eq,*/ int32_t handle, void* udata);
int32_t sceVideoOutSetFlipRate(int32_t handle, int32_t rate);
void sceVideoOutSetBufferAttribute(/* SceVideoOutBufferAttribute* attribute,*/ uint32_t pixelFormat, uint32_t tilingMode, uint32_t aspectRatio,
                                   uint32_t width, uint32_t height, uint32_t pitchInPixel);
int32_t sceVideoOutGetResolutionStatus(int32_t handle /*, SceVideoOutResolutionStatus* status*/);
int32_t PS4_SYSV_ABI sceVideoOutOpen(SceUserServiceUserId userId, int32_t busType, int32_t index, const void* param);
int32_t sceVideoOutIsFlipPending(int32_t handle);
};  // namespace HLE::Libs::LibSceVideoOut