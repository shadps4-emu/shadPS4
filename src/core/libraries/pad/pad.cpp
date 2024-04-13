// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/pad/pad.h"
#include "input/controller.h"

namespace Libraries::LibPad {

int PS4_SYSV_ABI scePadInit() {
    LOG_WARNING(Lib_Pad, "(DUMMY) called");
    return SCE_OK;
}

int PS4_SYSV_ABI scePadOpen(Libraries::UserService::OrbisUserServiceUserId user_id, s32 type,
                            s32 index, const ScePadOpenParam* pParam) {
    LOG_INFO(Lib_Pad, "(DUMMY) called user_id = {} type = {} index = {}", user_id, type, index);
    return 1; // dummy
}

int PS4_SYSV_ABI scePadReadState(int32_t handle, ScePadData* pData) {
    auto* controller = Common::Singleton<Input::GameController>::Instance();

    int connectedCount = 0;
    bool isConnected = false;
    Input::State state;

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

int PS4_SYSV_ABI scePadRead(int handle, ScePadData* pData, int num) {
    int connected_count = 0;
    bool connected = false;
    Input::State states[64];
    auto* controller = Common::Singleton<Input::GameController>::Instance();
    int ret_num = controller->ReadStates(states, num, &connected, &connected_count);

    if (!connected) {
        ret_num = 1;
    }

    for (int i = 0; i < ret_num; i++) {
        pData[i].buttons = states[i].buttonsState;
        pData[i].leftStick.x = 128;    // dummy
        pData[i].leftStick.y = 128;    // dummy
        pData[i].rightStick.x = 0;     // dummy
        pData[i].rightStick.y = 0;     // dummy
        pData[i].analogButtons.l2 = 0; // dummy
        pData[i].analogButtons.r2 = 0; // dummy
        pData[i].orientation.x = 0.0f;
        pData[i].orientation.y = 0.0f;
        pData[i].orientation.z = 0.0f;
        pData[i].orientation.w = 1.0f;
        pData[i].acceleration.x = 0.0f;
        pData[i].acceleration.y = 0.0f;
        pData[i].acceleration.z = 0.0f;
        pData[i].angularVelocity.x = 0.0f;
        pData[i].angularVelocity.y = 0.0f;
        pData[i].angularVelocity.z = 0.0f;
        pData[i].touchData.touchNum = 0;
        pData[i].touchData.touch[0].x = 0;
        pData[i].touchData.touch[0].y = 0;
        pData[i].touchData.touch[0].id = 1;
        pData[i].touchData.touch[1].x = 0;
        pData[i].touchData.touch[1].y = 0;
        pData[i].touchData.touch[1].id = 2;
        pData[i].connected = connected;
        pData[i].timestamp = states[i].time;
        pData[i].connectedCount = connected_count;
        pData[i].deviceUniqueDataLen = 0;
    }

    return ret_num;
}

s32 PS4_SYSV_ABI scePadGetControllerInformation(s32 handle, OrbisPadInformation* info) {
    LOG_INFO(Lib_Pad, "called handle = {}", handle);
    info->touchpadDensity = 1;
    info->touchResolutionX = 1920;
    info->touchResolutionY = 950;
    info->stickDeadzoneL = 2;
    info->stickDeadzoneR = 2;
    info->connectionType = ORBIS_PAD_CONNECTION_TYPE_STANDARD;
    info->count = 1;
    info->connected = 1;
    info->deviceClass = ORBIS_PAD_PORT_TYPE_STANDARD;
    return SCE_OK;
}

s32 PS4_SYSV_ABI scePadSetMotionSensorState(s32 handle, bool enable) {
    LOG_INFO(Lib_Pad, "(DUMMY) called handle = {} enabled = {}", handle,
             (enable ? "true" : "false"));
    return SCE_OK;
}

void padSymbolsRegister(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("hv1luiJrqQM", "libScePad", 1, "libScePad", 1, 1, scePadInit);
    LIB_FUNCTION("xk0AcarP3V4", "libScePad", 1, "libScePad", 1, 1, scePadOpen);
    LIB_FUNCTION("YndgXqQVV7c", "libScePad", 1, "libScePad", 1, 1, scePadReadState);
    LIB_FUNCTION("q1cHNfGycLI", "libScePad", 1, "libScePad", 1, 1, scePadRead);

    LIB_FUNCTION("gjP9-KQzoUk", "libScePad", 1, "libScePad", 1, 1, scePadGetControllerInformation);
    LIB_FUNCTION("clVvL4ZDntw", "libScePad", 1, "libScePad", 1, 1, scePadSetMotionSensorState);
}

} // namespace Libraries::LibPad
