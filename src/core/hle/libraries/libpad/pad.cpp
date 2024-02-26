// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Emulator/Host/controller.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/hle/error_codes.h"
#include "core/hle/libraries/libpad/pad.h"
#include "core/hle/libraries/libs.h"

namespace Core::Libraries::LibPad {

int PS4_SYSV_ABI scePadInit() {
    LOG_WARNING(Lib_Pad, "(STUBBED) called");
    return SCE_OK;
}

int PS4_SYSV_ABI scePadOpen(Core::Libraries::LibUserService::SceUserServiceUserId user_id, s32 type,
                            s32 index, const ScePadOpenParam* pParam) {
    LOG_INFO(Lib_Pad, "(STUBBED) called user_id = {} type = {} index = {}", user_id, type, index);
    return 1; // dummy
}

int PS4_SYSV_ABI scePadReadState(int32_t handle, ScePadData* pData) {
    auto* controller = Common::Singleton<Emulator::Host::Controller::GameController>::Instance();

    int connectedCount = 0;
    bool isConnected = false;
    Emulator::Host::Controller::State state;

    controller->readState(&state, &isConnected, &connectedCount);
    pData->buttons = state.buttonsState;
    pData->leftStick.x = 128;    // dummy
    pData->leftStick.y = 128;    // dummy
    pData->rightStick.x = 0;     // dummy
    pData->rightStick.y = 0;     // dummy
    pData->analogButtons.r2 = 0; // dummy
    pData->analogButtons.l2 = 0; // dummy
    pData->orientation.x = 0;
    pData->orientation.y = 0;
    pData->orientation.z = 0;
    pData->orientation.w = 0;
    pData->timestamp = state.time;
    pData->connected = true;   // isConnected; //TODO fix me proper
    pData->connectedCount = 1; // connectedCount;
    pData->deviceUniqueDataLen = 0;

    return SCE_OK;
}

void padSymbolsRegister(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("hv1luiJrqQM", "libScePad", 1, "libScePad", 1, 1, scePadInit);
    LIB_FUNCTION("xk0AcarP3V4", "libScePad", 1, "libScePad", 1, 1, scePadOpen);
    LIB_FUNCTION("YndgXqQVV7c", "libScePad", 1, "libScePad", 1, 1, scePadReadState);
}

} // namespace Core::Libraries::LibPad
