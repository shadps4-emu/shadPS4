#pragma once
#include "../Loader/SymbolsResolver.h"

namespace HLE::Libs::LibSceVideoOut {

using SceUserServiceUserId = s32; //TODO move it to proper place

void LibSceVideoOut_Register(SymbolsResolver* sym);
//functions
int32_t PS4_SYSV_ABI sceVideoOutGetFlipStatus(int32_t handle /*, SceVideoOutFlipStatus* status*/);
int32_t PS4_SYSV_ABI sceVideoOutSubmitFlip(int32_t handle, int32_t bufferIndex, int32_t flipMode, int64_t flipArg);
int32_t PS4_SYSV_ABI sceVideoOutRegisterBuffers(int32_t handle, int32_t startIndex, void* const* addresses,
                                                int32_t bufferNum /*,const SceVideoOutBufferAttribute* attribute*/);
int32_t PS4_SYSV_ABI sceVideoOutAddFlipEvent(/*SceKernelEqueue eq,*/ int32_t handle, void* udata);
int32_t PS4_SYSV_ABI sceVideoOutSetFlipRate(int32_t handle, int32_t rate);

int32_t PS4_SYSV_ABI  sceVideoOutGetResolutionStatus(int32_t handle /*, SceVideoOutResolutionStatus* status*/);
s32 PS4_SYSV_ABI sceVideoOutOpen(SceUserServiceUserId userId, s32 busType, s32 index, const void* param);
int32_t PS4_SYSV_ABI sceVideoOutIsFlipPending(int32_t handle);
};  // namespace HLE::Libs::LibSceVideoOut