#pragma once
#include "core/PS4/Loader/SymbolsResolver.h"

namespace Emulator::HLE::Libraries::LibUserService {

using SceUserServiceUserId = s32;

struct SceUserServiceInitializeParams {
    s32 priority;
};

struct SceUserServiceLoginUserIdList {
    int user_id[4];
};

s32 PS4_SYSV_ABI sceUserServiceInitialize(const SceUserServiceInitializeParams* initParams);
s32 PS4_SYSV_ABI sceUserServiceGetLoginUserIdList(SceUserServiceLoginUserIdList* userIdList);

void libUserService_Register(SymbolsResolver* sym);
};  // namespace Emulator::HLE::Libraries::LibUserService