#include "pad.h"

#include <Core/PS4/HLE/ErrorCodes.h>
#include <Core/PS4/HLE/Libs.h>

#include "Emulator/Util/singleton.h"
#include "controller.h"
#include <debug.h>
#include <Util/log.h>

namespace Emulator::HLE::Libraries::LibPad {

constexpr bool log_file_pad = true;  // disable it to disable logging

int PS4_SYSV_ABI scePadInit() { return SCE_OK; }

int PS4_SYSV_ABI scePadOpen(Emulator::HLE::Libraries::LibUserService::SceUserServiceUserId userId, s32 type, s32 index,
                            const ScePadOpenParam* pParam) {
    return 1;  // dummy
}

int PS4_SYSV_ABI scePadReadState(int32_t handle, ScePadData* pData) {
    auto* controller = singleton<Emulator::Host::Controller::GameController>::instance();

    int connectedCount = 0;
    bool isConnected = false;
    Emulator::Host::Controller::State state;

    controller->readState(&state, &isConnected, &connectedCount);
    pData->buttons = state.buttonsState;
    pData->leftStick.x = 128;   // dummy
    pData->leftStick.y = 128;   // dummy
    pData->rightStick.x = 0;  // dummy
    pData->rightStick.y = 0;  // dummy
    pData->analogButtons.r2 = 0;//dummy
    pData->analogButtons.l2 = 0;//dummy
    pData->orientation.x = 0;
    pData->orientation.y = 0;
    pData->orientation.z = 0;
    pData->orientation.w = 0;

    pData->connected = true;  // isConnected; //TODO fix me proper
    pData->connectedCount = 1;//connectedCount;
    pData->deviceUniqueDataLen = 0;

    return SCE_OK;
}

void libPad_Register(SymbolsResolver* sym) {
    LIB_FUNCTION("hv1luiJrqQM", "libScePad", 1, "libScePad", 1, 1, scePadInit);
    LIB_FUNCTION("xk0AcarP3V4", "libScePad", 1, "libScePad", 1, 1, scePadOpen);
    LIB_FUNCTION("YndgXqQVV7c", "libScePad", 1, "libScePad", 1, 1, scePadReadState);
}

}  // namespace Emulator::HLE::Libraries::LibPad
