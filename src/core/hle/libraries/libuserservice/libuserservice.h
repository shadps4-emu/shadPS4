#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibUserService {

using SceUserServiceUserId = s32;

enum UserServiceEventType { UserServiceEventTypeLogin, UserServiceEventTypeLogout };

struct SceUserServiceEvent {
    UserServiceEventType event_type;
    int user_id;
};

struct SceUserServiceInitializeParams {
    s32 priority;
};

struct SceUserServiceLoginUserIdList {
    int user_id[4];
};

s32 PS4_SYSV_ABI sceUserServiceInitialize(const SceUserServiceInitializeParams* initParams);
s32 PS4_SYSV_ABI sceUserServiceGetLoginUserIdList(SceUserServiceLoginUserIdList* userIdList);
s32 PS4_SYSV_ABI sceUserServiceGetInitialUser(SceUserServiceUserId* userId);

void userServiceSymbolsRegister(Loader::SymbolsResolver* sym);

}; // namespace Core::Libraries::LibUserService
