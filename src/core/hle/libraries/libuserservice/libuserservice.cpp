#include "core/hle/libraries/libuserservice/libuserservice.h"

#include "common/log.h"
#include "core/hle/error_codes.h"
#include "core/hle/libraries/libs.h"
#include "usr_mng_codes.h"

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

s32 PS4_SYSV_ABI sceUserServiceGetInitialUser(SceUserServiceUserId* userId) {
    PRINT_DUMMY_FUNCTION_NAME();
    *userId = 1;  // first player only
    return SCE_OK;
}

int PS4_SYSV_ABI sceUserServiceGetEvent(SceUserServiceEvent* event) {
    PRINT_DUMMY_FUNCTION_NAME();

    static bool logged_in = false;

    if (!logged_in) {
        logged_in = true;
        event->event_type = UserServiceEventTypeLogin;
        event->user_id = 1;
        return SCE_OK;
    }

    return SCE_USER_SERVICE_ERROR_NO_EVENT;
}

void userServiceSymbolsRegister(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("j3YMu1MVNNo", "libSceUserService", 1, "libSceUserService", 1, 1, sceUserServiceInitialize);
    LIB_FUNCTION("fPhymKNvK-A", "libSceUserService", 1, "libSceUserService", 1, 1, sceUserServiceGetLoginUserIdList);
    LIB_FUNCTION("CdWp0oHWGr0", "libSceUserService", 1, "libSceUserService", 1, 1, sceUserServiceGetInitialUser);
    LIB_FUNCTION("yH17Q6NWtVg", "libSceUserService", 1, "libSceUserService", 1, 1, sceUserServiceGetEvent);
}

}  // namespace Core::Libraries::LibUserService
