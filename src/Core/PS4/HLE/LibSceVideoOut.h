#pragma once
#include "../Loader/SymbolsResolver.h"

namespace HLE::Libs::LibSceVideoOut {

using SceUserServiceUserId = s32; //TODO move it to proper place

void LibSceVideoOut_Register(SymbolsResolver* sym);
//functions
s32 PS4_SYSV_ABI sceVideoOutOpen(SceUserServiceUserId userId, s32 busType, s32 index, const void* param);
};  // namespace HLE::Libs::LibSceVideoOut