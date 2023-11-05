#include "user_service.h"

#include <core/PS4/HLE/ErrorCodes.h>
#include <core/PS4/HLE/Libs.h>

#include "common/log.h"

namespace Core::Libraries::LibUserService {

s32 PS4_SYSV_ABI sceUserServiceInitialize(const SceUserServiceInitializeParams* initParams) {
    PRINT_DUMMY_FUNCTION_NAME();
    return SCE_OK;
}

s32 PS4_SYSV_ABI sceUserServiceGetLoginUserIdList(SceUserServiceLoginUserIdList* userIdList) {
    PRINT_DUMMY_FUNCTION_NAME();
    userIdList->user_id[0] = 1;
    userIdList->user_id[1] = -1;
    userIdList->user_id[2] = -1;
    userIdList->user_id[3] = -1;

    return SCE_OK;
}
void userServiceSymbolsRegister(SymbolsResolver* sym) {
    LIB_FUNCTION("j3YMu1MVNNo", "libSceUserService", 1, "libSceUserService", 1, 1, sceUserServiceInitialize);
    LIB_FUNCTION("fPhymKNvK-A", "libSceUserService", 1, "libSceUserService", 1, 1, sceUserServiceGetLoginUserIdList);
}

}  // namespace Core::Libraries::LibUserService
