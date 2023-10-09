#include "pad.h"

#include <Core/PS4/HLE/ErrorCodes.h>
#include <Core/PS4/HLE/Libs.h>

namespace Emulator::HLE::Libraries::LibPad {
int PS4_SYSV_ABI scePadInit() { return SCE_OK; }

int PS4_SYSV_ABI scePadOpen(Emulator::HLE::Libraries::LibUserService::SceUserServiceUserId userId, s32 type, s32 index,
                            const ScePadOpenParam* pParam) {
    return 1;  // dummy
}

int PS4_SYSV_ABI scePadReadState(int32_t handle, ScePadData* pData) {
    pData->connected = true;  // make it think it is connected
    return SCE_OK;
}

void libPad_Register(SymbolsResolver* sym) {
    LIB_FUNCTION("hv1luiJrqQM", "libScePad", 1, "libScePad", 1, 1, scePadInit);
    LIB_FUNCTION("xk0AcarP3V4", "libScePad", 1, "libScePad", 1, 1, scePadOpen);
    LIB_FUNCTION("YndgXqQVV7c", "libScePad", 1, "libScePad", 1, 1, scePadReadState);
}

}  // namespace Emulator::HLE::Libraries::LibPad
