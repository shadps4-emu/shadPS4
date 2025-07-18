// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/libs.h"
#include "core/libraries/pad/pad_errors.h"
#include "input/controller.h"
#include "pad.h"

namespace Libraries::Pad {

using Input::GameController;

int PS4_SYSV_ABI scePadClose(s32 handle) {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadConnectPort() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadDeviceClassGetExtendedInformation(
    s32 handle, OrbisPadDeviceClassExtendedInformation* pExtInfo) {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    std::memset(pExtInfo, 0, sizeof(OrbisPadDeviceClassExtendedInformation));
    if (Config::getUseSpecialPad()) {
        pExtInfo->deviceClass = (OrbisPadDeviceClass)Config::getSpecialPadClass();
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadDeviceClassParseData(s32 handle, const OrbisPadData* pData,
                                            OrbisPadDeviceClassData* pDeviceClassData) {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadDeviceOpen() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadDisableVibration() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadDisconnectDevice() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadDisconnectPort() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadEnableAutoDetect() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadEnableExtensionPort() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadEnableSpecificDeviceClass() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadEnableUsbConnection() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetBluetoothAddress() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetCapability() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetControllerInformation(s32 handle, OrbisPadControllerInformation* pInfo) {
    LOG_DEBUG(Lib_Pad, "called handle = {}", handle);
    if (handle < 0) {
        pInfo->touchPadInfo.pixelDensity = 1;
        pInfo->touchPadInfo.resolution.x = 1920;
        pInfo->touchPadInfo.resolution.y = 950;
        pInfo->stickInfo.deadZoneLeft = 1;
        pInfo->stickInfo.deadZoneRight = 1;
        pInfo->connectionType = ORBIS_PAD_PORT_TYPE_STANDARD;
        pInfo->connectedCount = 1;
        pInfo->connected = false;
        pInfo->deviceClass = OrbisPadDeviceClass::Standard;
        return ORBIS_OK;
    }
    pInfo->touchPadInfo.pixelDensity = 1;
    pInfo->touchPadInfo.resolution.x = 1920;
    pInfo->touchPadInfo.resolution.y = 950;
    pInfo->stickInfo.deadZoneLeft = 1;
    pInfo->stickInfo.deadZoneRight = 1;
    pInfo->connectionType = ORBIS_PAD_PORT_TYPE_STANDARD;
    pInfo->connectedCount = 1;
    pInfo->connected = true;
    pInfo->deviceClass = OrbisPadDeviceClass::Standard;
    if (Config::getUseSpecialPad()) {
        pInfo->connectionType = ORBIS_PAD_PORT_TYPE_SPECIAL;
        pInfo->deviceClass = (OrbisPadDeviceClass)Config::getSpecialPadClass();
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetDataInternal() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetDeviceId() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetDeviceInfo() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetExtControllerInformation(s32 handle,
                                                   OrbisPadExtendedControllerInformation* pInfo) {
    LOG_INFO(Lib_Pad, "called handle = {}", handle);

    pInfo->padType1 = 0;
    pInfo->padType2 = 0;
    pInfo->capability = 0;

    auto res = scePadGetControllerInformation(handle, &pInfo->base);
    return res;
}

int PS4_SYSV_ABI scePadGetExtensionUnitInfo() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetFeatureReport() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetHandle(s32 userId, s32 type, s32 index) {
    if (userId == -1) {
        return ORBIS_PAD_ERROR_DEVICE_NO_HANDLE;
    }
    LOG_DEBUG(Lib_Pad, "(DUMMY) called");
    return 1;
}

int PS4_SYSV_ABI scePadGetIdleCount() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetInfo() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetInfoByPortType() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetLicenseControllerInformation() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetMotionSensorPosition() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetMotionTimerUnit() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetSphereRadius() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadGetVersionInfo() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadInit() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadIsBlasterConnected() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadIsDS4Connected() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadIsLightBarBaseBrightnessControllable() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadIsMoveConnected() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadIsMoveReproductionModel() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadIsValidHandle() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadMbusInit() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadMbusTerm() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadOpen(s32 userId, s32 type, s32 index, const OrbisPadOpenParam* pParam) {
    if (userId == -1) {
        return ORBIS_PAD_ERROR_DEVICE_NO_HANDLE;
    }
    if (Config::getUseSpecialPad()) {
        if (type != ORBIS_PAD_PORT_TYPE_SPECIAL)
            return ORBIS_PAD_ERROR_DEVICE_NOT_CONNECTED;
    } else {
        if (type != ORBIS_PAD_PORT_TYPE_STANDARD && type != ORBIS_PAD_PORT_TYPE_REMOTE_CONTROL)
            return ORBIS_PAD_ERROR_DEVICE_NOT_CONNECTED;
    }
    LOG_INFO(Lib_Pad, "(DUMMY) called user_id = {} type = {} index = {}", userId, type, index);
    scePadResetLightBar(1);
    return 1; // dummy
}

int PS4_SYSV_ABI scePadOpenExt(s32 userId, s32 type, s32 index,
                               const OrbisPadOpenExtParam* pParam) {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    if (Config::getUseSpecialPad()) {
        if (type != ORBIS_PAD_PORT_TYPE_SPECIAL)
            return ORBIS_PAD_ERROR_DEVICE_NOT_CONNECTED;
    } else {
        if (type != ORBIS_PAD_PORT_TYPE_STANDARD && type != ORBIS_PAD_PORT_TYPE_REMOTE_CONTROL)
            return ORBIS_PAD_ERROR_DEVICE_NOT_CONNECTED;
    }
    return 1; // dummy
}

int PS4_SYSV_ABI scePadOpenExt2() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadOutputReport() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadRead(s32 handle, OrbisPadData* pData, s32 num) {
    LOG_TRACE(Lib_Pad, "called");
    int connected_count = 0;
    bool connected = false;
    Input::State states[64];
    auto* controller = Common::Singleton<GameController>::Instance();
    const auto* engine = controller->GetEngine();
    int ret_num = controller->ReadStates(states, num, &connected, &connected_count);

    if (!connected) {
        ret_num = 1;
    }

    for (int i = 0; i < ret_num; i++) {
        pData[i].buttons = states[i].buttonsState;
        pData[i].leftStick.x = states[i].axes[static_cast<int>(Input::Axis::LeftX)];
        pData[i].leftStick.y = states[i].axes[static_cast<int>(Input::Axis::LeftY)];
        pData[i].rightStick.x = states[i].axes[static_cast<int>(Input::Axis::RightX)];
        pData[i].rightStick.y = states[i].axes[static_cast<int>(Input::Axis::RightY)];
        pData[i].analogButtons.l2 = states[i].axes[static_cast<int>(Input::Axis::TriggerLeft)];
        pData[i].analogButtons.r2 = states[i].axes[static_cast<int>(Input::Axis::TriggerRight)];
        pData[i].acceleration.x = states[i].acceleration.x;
        pData[i].acceleration.y = states[i].acceleration.y;
        pData[i].acceleration.z = states[i].acceleration.z;
        pData[i].angularVelocity.x = states[i].angularVelocity.x;
        pData[i].angularVelocity.y = states[i].angularVelocity.y;
        pData[i].angularVelocity.z = states[i].angularVelocity.z;
        pData[i].orientation = {0.0f, 0.0f, 0.0f, 1.0f};
        pData[i].acceleration.x = states[i].acceleration.x * 0.098;
        pData[i].acceleration.y = states[i].acceleration.y * 0.098;
        pData[i].acceleration.z = states[i].acceleration.z * 0.098;
        pData[i].angularVelocity.x = states[i].angularVelocity.x;
        pData[i].angularVelocity.y = states[i].angularVelocity.y;
        pData[i].angularVelocity.z = states[i].angularVelocity.z;

        if (engine && handle == 1) {
            const auto gyro_poll_rate = engine->GetAccelPollRate();
            if (gyro_poll_rate != 0.0f) {
                auto now = std::chrono::steady_clock::now();
                float deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(
                                      now - controller->GetLastUpdate())
                                      .count() /
                                  1000000.0f;
                controller->SetLastUpdate(now);
                Libraries::Pad::OrbisFQuaternion lastOrientation = controller->GetLastOrientation();
                Libraries::Pad::OrbisFQuaternion outputOrientation = {0.0f, 0.0f, 0.0f, 1.0f};
                GameController::CalculateOrientation(pData->acceleration, pData->angularVelocity,
                                                     deltaTime, lastOrientation, outputOrientation);
                pData[i].orientation = outputOrientation;
                controller->SetLastOrientation(outputOrientation);
            }
        }

        pData[i].touchData.touchNum =
            (states[i].touchpad[0].state ? 1 : 0) + (states[i].touchpad[1].state ? 1 : 0);

        if (handle == 1) {
            if (controller->GetTouchCount() >= 127) {
                controller->SetTouchCount(0);
            }

            if (controller->GetSecondaryTouchCount() >= 127) {
                controller->SetSecondaryTouchCount(0);
            }

            if (pData->touchData.touchNum == 1 && controller->GetPreviousTouchNum() == 0) {
                controller->SetTouchCount(controller->GetTouchCount() + 1);
                controller->SetSecondaryTouchCount(controller->GetTouchCount());
            } else if (pData->touchData.touchNum == 2 && controller->GetPreviousTouchNum() == 1) {
                controller->SetSecondaryTouchCount(controller->GetSecondaryTouchCount() + 1);
            } else if (pData->touchData.touchNum == 0 && controller->GetPreviousTouchNum() > 0) {
                if (controller->GetTouchCount() < controller->GetSecondaryTouchCount()) {
                    controller->SetTouchCount(controller->GetSecondaryTouchCount());
                } else {
                    if (controller->WasSecondaryTouchReset()) {
                        controller->SetTouchCount(controller->GetSecondaryTouchCount());
                        controller->UnsetSecondaryTouchResetBool();
                    }
                }
            }

            controller->SetPreviousTouchNum(pData->touchData.touchNum);

            if (pData->touchData.touchNum == 1) {
                states[i].touchpad[0].ID = controller->GetTouchCount();
                states[i].touchpad[1].ID = 0;
            } else if (pData->touchData.touchNum == 2) {
                states[i].touchpad[0].ID = controller->GetTouchCount();
                states[i].touchpad[1].ID = controller->GetSecondaryTouchCount();
            }
        } else {
            states[i].touchpad[0].ID = 1;
            states[i].touchpad[1].ID = 2;
        }

        pData[i].touchData.touch[0].x = states[i].touchpad[0].x;
        pData[i].touchData.touch[0].y = states[i].touchpad[0].y;
        pData[i].touchData.touch[0].id = states[i].touchpad[0].ID;
        pData[i].touchData.touch[1].x = states[i].touchpad[1].x;
        pData[i].touchData.touch[1].y = states[i].touchpad[1].y;
        pData[i].touchData.touch[1].id = states[i].touchpad[1].ID;
        pData[i].connected = connected;
        pData[i].timestamp = states[i].time;
        pData[i].connectedCount = connected_count;
        pData[i].deviceUniqueDataLen = 0;
    }

    return ret_num;
}

int PS4_SYSV_ABI scePadReadBlasterForTracker() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadReadExt() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadReadForTracker() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadReadHistory() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadReadState(s32 handle, OrbisPadData* pData) {
    LOG_TRACE(Lib_Pad, "called");
    if (handle == ORBIS_PAD_ERROR_DEVICE_NO_HANDLE) {
        return ORBIS_PAD_ERROR_INVALID_HANDLE;
    }
    auto* controller = Common::Singleton<GameController>::Instance();
    const auto* engine = controller->GetEngine();
    int connectedCount = 0;
    bool isConnected = false;
    Input::State state;
    controller->ReadState(&state, &isConnected, &connectedCount);
    pData->buttons = state.buttonsState;
    pData->leftStick.x = state.axes[static_cast<int>(Input::Axis::LeftX)];
    pData->leftStick.y = state.axes[static_cast<int>(Input::Axis::LeftY)];
    pData->rightStick.x = state.axes[static_cast<int>(Input::Axis::RightX)];
    pData->rightStick.x = state.axes[static_cast<int>(Input::Axis::RightX)];
    pData->rightStick.y = state.axes[static_cast<int>(Input::Axis::RightY)];
    pData->analogButtons.l2 = state.axes[static_cast<int>(Input::Axis::TriggerLeft)];
    pData->analogButtons.r2 = state.axes[static_cast<int>(Input::Axis::TriggerRight)];
    pData->acceleration.x = state.acceleration.x * 0.098;
    pData->acceleration.y = state.acceleration.y * 0.098;
    pData->acceleration.z = state.acceleration.z * 0.098;
    pData->angularVelocity.x = state.angularVelocity.x;
    pData->angularVelocity.y = state.angularVelocity.y;
    pData->angularVelocity.z = state.angularVelocity.z;
    pData->orientation = {0.0f, 0.0f, 0.0f, 1.0f};

    // Only do this on handle 1 for now
    if (engine && handle == 1) {
        auto now = std::chrono::steady_clock::now();
        float deltaTime =
            std::chrono::duration_cast<std::chrono::microseconds>(now - controller->GetLastUpdate())
                .count() /
            1000000.0f;
        controller->SetLastUpdate(now);
        Libraries::Pad::OrbisFQuaternion lastOrientation = controller->GetLastOrientation();
        Libraries::Pad::OrbisFQuaternion outputOrientation = {0.0f, 0.0f, 0.0f, 1.0f};
        GameController::CalculateOrientation(pData->acceleration, pData->angularVelocity, deltaTime,
                                             lastOrientation, outputOrientation);
        pData->orientation = outputOrientation;
        controller->SetLastOrientation(outputOrientation);
    }
    pData->touchData.touchNum =
        (state.touchpad[0].state ? 1 : 0) + (state.touchpad[1].state ? 1 : 0);

    // Only do this on handle 1 for now
    if (handle == 1) {
        if (controller->GetTouchCount() >= 127) {
            controller->SetTouchCount(0);
        }

        if (controller->GetSecondaryTouchCount() >= 127) {
            controller->SetSecondaryTouchCount(0);
        }

        if (pData->touchData.touchNum == 1 && controller->GetPreviousTouchNum() == 0) {
            controller->SetTouchCount(controller->GetTouchCount() + 1);
            controller->SetSecondaryTouchCount(controller->GetTouchCount());
        } else if (pData->touchData.touchNum == 2 && controller->GetPreviousTouchNum() == 1) {
            controller->SetSecondaryTouchCount(controller->GetSecondaryTouchCount() + 1);
        } else if (pData->touchData.touchNum == 0 && controller->GetPreviousTouchNum() > 0) {
            if (controller->GetTouchCount() < controller->GetSecondaryTouchCount()) {
                controller->SetTouchCount(controller->GetSecondaryTouchCount());
            } else {
                if (controller->WasSecondaryTouchReset()) {
                    controller->SetTouchCount(controller->GetSecondaryTouchCount());
                    controller->UnsetSecondaryTouchResetBool();
                }
            }
        }

        controller->SetPreviousTouchNum(pData->touchData.touchNum);

        if (pData->touchData.touchNum == 1) {
            state.touchpad[0].ID = controller->GetTouchCount();
            state.touchpad[1].ID = 0;
        } else if (pData->touchData.touchNum == 2) {
            state.touchpad[0].ID = controller->GetTouchCount();
            state.touchpad[1].ID = controller->GetSecondaryTouchCount();
        }
    } else {
        state.touchpad[0].ID = 1;
        state.touchpad[1].ID = 2;
    }

    pData->touchData.touch[0].x = state.touchpad[0].x;
    pData->touchData.touch[0].y = state.touchpad[0].y;
    pData->touchData.touch[0].id = state.touchpad[0].ID;
    pData->touchData.touch[1].x = state.touchpad[1].x;
    pData->touchData.touch[1].y = state.touchpad[1].y;
    pData->touchData.touch[1].id = state.touchpad[1].ID;
    pData->timestamp = state.time;
    pData->connected = true;   // isConnected; //TODO fix me proper
    pData->connectedCount = 1; // connectedCount;
    pData->deviceUniqueDataLen = 0;

    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadReadStateExt() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadResetLightBar(s32 handle) {
    LOG_INFO(Lib_Pad, "(DUMMY) called");
    if (handle != 1) {
        return ORBIS_PAD_ERROR_INVALID_HANDLE;
    }
    auto* controller = Common::Singleton<GameController>::Instance();
    int* rgb = Config::GetControllerCustomColor();
    controller->SetLightBarRGB(rgb[0], rgb[1], rgb[2]);
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadResetLightBarAll() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadResetLightBarAllByPortType() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadResetOrientation(s32 handle) {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadResetOrientationForTracker() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetAngularVelocityDeadbandState(s32 handle, bool bEnable) {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetAutoPowerOffCount() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetButtonRemappingInfo() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetConnection() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetExtensionReport() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetFeatureReport() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetForceIntercepted() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetLightBar(s32 handle, const OrbisPadLightBarParam* pParam) {
    if (Config::GetOverrideControllerColor()) {
        return ORBIS_OK;
    }
    if (pParam != nullptr) {
        LOG_DEBUG(Lib_Pad, "called handle = {} rgb = {} {} {}", handle, pParam->r, pParam->g,
                  pParam->b);

        if (pParam->r < 0xD && pParam->g < 0xD && pParam->b < 0xD) {
            LOG_INFO(Lib_Pad, "Invalid lightbar setting");
            return ORBIS_PAD_ERROR_INVALID_LIGHTBAR_SETTING;
        }

        auto* controller = Common::Singleton<GameController>::Instance();
        controller->SetLightBarRGB(pParam->r, pParam->g, pParam->b);
        return ORBIS_OK;
    }
    return ORBIS_PAD_ERROR_INVALID_ARG;
}

int PS4_SYSV_ABI scePadSetLightBarBaseBrightness() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetLightBarBlinking() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetLightBarForTracker() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetLoginUserNumber() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetMotionSensorState(s32 handle, bool bEnable) {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
    // it's already handled by the SDL backend and will be on no matter what
    // (assuming the controller supports it)
}

int PS4_SYSV_ABI scePadSetProcessFocus() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetProcessPrivilege() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetProcessPrivilegeOfButtonRemapping() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetShareButtonMaskForRemotePlay() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetTiltCorrectionState(s32 handle, bool bEnable) {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetUserColor() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetVibration(s32 handle, const OrbisPadVibrationParam* pParam) {
    if (pParam != nullptr) {
        LOG_DEBUG(Lib_Pad, "scePadSetVibration called handle = {} data = {} , {}", handle,
                  pParam->smallMotor, pParam->largeMotor);
        auto* controller = Common::Singleton<GameController>::Instance();
        controller->SetVibration(pParam->smallMotor, pParam->largeMotor);
        return ORBIS_OK;
    }
    return ORBIS_PAD_ERROR_INVALID_ARG;
}

int PS4_SYSV_ABI scePadSetVibrationForce() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSetVrTrackingMode() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadShareOutputData() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadStartRecording() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadStopRecording() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadSwitchConnection() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadVertualDeviceAddDevice() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadVirtualDeviceAddDevice() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadVirtualDeviceDeleteDevice() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadVirtualDeviceDisableButtonRemapping() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadVirtualDeviceGetRemoteSetting() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadVirtualDeviceInsertData() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_28B998C7D8A3DA1D() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_298D21481F94C9FA() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_51E514BCD3A05CA5() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_89C9237E393DA243() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_EF103E845B6F0420() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("6ncge5+l5Qs", "libScePad", 1, "libScePad", 1, 1, scePadClose);
    LIB_FUNCTION("kazv1NzSB8c", "libScePad", 1, "libScePad", 1, 1, scePadConnectPort);
    LIB_FUNCTION("AcslpN1jHR8", "libScePad", 1, "libScePad", 1, 1,
                 scePadDeviceClassGetExtendedInformation);
    LIB_FUNCTION("IHPqcbc0zCA", "libScePad", 1, "libScePad", 1, 1, scePadDeviceClassParseData);
    LIB_FUNCTION("d7bXuEBycDI", "libScePad", 1, "libScePad", 1, 1, scePadDeviceOpen);
    LIB_FUNCTION("0aziJjRZxqQ", "libScePad", 1, "libScePad", 1, 1, scePadDisableVibration);
    LIB_FUNCTION("pnZXireDoeI", "libScePad", 1, "libScePad", 1, 1, scePadDisconnectDevice);
    LIB_FUNCTION("9ez71nWSvD0", "libScePad", 1, "libScePad", 1, 1, scePadDisconnectPort);
    LIB_FUNCTION("77ooWxGOIVs", "libScePad", 1, "libScePad", 1, 1, scePadEnableAutoDetect);
    LIB_FUNCTION("+cE4Jx431wc", "libScePad", 1, "libScePad", 1, 1, scePadEnableExtensionPort);
    LIB_FUNCTION("E1KEw5XMGQQ", "libScePad", 1, "libScePad", 1, 1, scePadEnableSpecificDeviceClass);
    LIB_FUNCTION("DD-KiRLBqkQ", "libScePad", 1, "libScePad", 1, 1, scePadEnableUsbConnection);
    LIB_FUNCTION("Q66U8FdrMaw", "libScePad", 1, "libScePad", 1, 1, scePadGetBluetoothAddress);
    LIB_FUNCTION("qtasqbvwgV4", "libScePad", 1, "libScePad", 1, 1, scePadGetCapability);
    LIB_FUNCTION("gjP9-KQzoUk", "libScePad", 1, "libScePad", 1, 1, scePadGetControllerInformation);
    LIB_FUNCTION("Uq6LgTJEmQs", "libScePad", 1, "libScePad", 1, 1, scePadGetDataInternal);
    LIB_FUNCTION("hDgisSGkOgw", "libScePad", 1, "libScePad", 1, 1, scePadGetDeviceId);
    LIB_FUNCTION("4rS5zG7RFaM", "libScePad", 1, "libScePad", 1, 1, scePadGetDeviceInfo);
    LIB_FUNCTION("hGbf2QTBmqc", "libScePad", 1, "libScePad", 1, 1,
                 scePadGetExtControllerInformation);
    LIB_FUNCTION("1DmZjZAuzEM", "libScePad", 1, "libScePad", 1, 1, scePadGetExtensionUnitInfo);
    LIB_FUNCTION("PZSoY8j0Pko", "libScePad", 1, "libScePad", 1, 1, scePadGetFeatureReport);
    LIB_FUNCTION("u1GRHp+oWoY", "libScePad", 1, "libScePad", 1, 1, scePadGetHandle);
    LIB_FUNCTION("kiA9bZhbnAg", "libScePad", 1, "libScePad", 1, 1, scePadGetIdleCount);
    LIB_FUNCTION("1Odcw19nADw", "libScePad", 1, "libScePad", 1, 1, scePadGetInfo);
    LIB_FUNCTION("4x5Im8pr0-4", "libScePad", 1, "libScePad", 1, 1, scePadGetInfoByPortType);
    LIB_FUNCTION("vegw8qax5MI", "libScePad", 1, "libScePad", 1, 1,
                 scePadGetLicenseControllerInformation);
    LIB_FUNCTION("WPIB7zBWxVE", "libScePad", 1, "libScePad", 1, 1, scePadGetMotionSensorPosition);
    LIB_FUNCTION("k4+nDV9vbT0", "libScePad", 1, "libScePad", 1, 1, scePadGetMotionTimerUnit);
    LIB_FUNCTION("do-JDWX+zRs", "libScePad", 1, "libScePad", 1, 1, scePadGetSphereRadius);
    LIB_FUNCTION("QuOaoOcSOw0", "libScePad", 1, "libScePad", 1, 1, scePadGetVersionInfo);
    LIB_FUNCTION("hv1luiJrqQM", "libScePad", 1, "libScePad", 1, 1, scePadInit);
    LIB_FUNCTION("bi0WNvZ1nug", "libScePad", 1, "libScePad", 1, 1, scePadIsBlasterConnected);
    LIB_FUNCTION("mEC+xJKyIjQ", "libScePad", 1, "libScePad", 1, 1, scePadIsDS4Connected);
    LIB_FUNCTION("d2Qk-i8wGak", "libScePad", 1, "libScePad", 1, 1,
                 scePadIsLightBarBaseBrightnessControllable);
    LIB_FUNCTION("4y9RNPSBsqg", "libScePad", 1, "libScePad", 1, 1, scePadIsMoveConnected);
    LIB_FUNCTION("9e56uLgk5y0", "libScePad", 1, "libScePad", 1, 1, scePadIsMoveReproductionModel);
    LIB_FUNCTION("pFTi-yOrVeQ", "libScePad", 1, "libScePad", 1, 1, scePadIsValidHandle);
    LIB_FUNCTION("CfwUlQtCFi4", "libScePad", 1, "libScePad", 1, 1, scePadMbusInit);
    LIB_FUNCTION("s7CvzS+9ZIs", "libScePad", 1, "libScePad", 1, 1, scePadMbusTerm);
    LIB_FUNCTION("xk0AcarP3V4", "libScePad", 1, "libScePad", 1, 1, scePadOpen);
    LIB_FUNCTION("WFIiSfXGUq8", "libScePad", 1, "libScePad", 1, 1, scePadOpenExt);
    LIB_FUNCTION("71E9e6n+2R8", "libScePad", 1, "libScePad", 1, 1, scePadOpenExt2);
    LIB_FUNCTION("DrUu8cPrje8", "libScePad", 1, "libScePad", 1, 1, scePadOutputReport);
    LIB_FUNCTION("q1cHNfGycLI", "libScePad", 1, "libScePad", 1, 1, scePadRead);
    LIB_FUNCTION("fm1r2vv5+OU", "libScePad", 1, "libScePad", 1, 1, scePadReadBlasterForTracker);
    LIB_FUNCTION("QjwkT2Ycmew", "libScePad", 1, "libScePad", 1, 1, scePadReadExt);
    LIB_FUNCTION("2NhkFTRnXHk", "libScePad", 1, "libScePad", 1, 1, scePadReadForTracker);
    LIB_FUNCTION("3u4M8ck9vJM", "libScePad", 1, "libScePad", 1, 1, scePadReadHistory);
    LIB_FUNCTION("YndgXqQVV7c", "libScePad", 1, "libScePad", 1, 1, scePadReadState);
    LIB_FUNCTION("5Wf4q349s+Q", "libScePad", 1, "libScePad", 1, 1, scePadReadStateExt);
    LIB_FUNCTION("DscD1i9HX1w", "libScePad", 1, "libScePad", 1, 1, scePadResetLightBar);
    LIB_FUNCTION("+4c9xRLmiXQ", "libScePad", 1, "libScePad", 1, 1, scePadResetLightBarAll);
    LIB_FUNCTION("+Yp6+orqf1M", "libScePad", 1, "libScePad", 1, 1,
                 scePadResetLightBarAllByPortType);
    LIB_FUNCTION("rIZnR6eSpvk", "libScePad", 1, "libScePad", 1, 1, scePadResetOrientation);
    LIB_FUNCTION("jbAqAvLEP4A", "libScePad", 1, "libScePad", 1, 1,
                 scePadResetOrientationForTracker);
    LIB_FUNCTION("r44mAxdSG+U", "libScePad", 1, "libScePad", 1, 1,
                 scePadSetAngularVelocityDeadbandState);
    LIB_FUNCTION("ew647HuKi2Y", "libScePad", 1, "libScePad", 1, 1, scePadSetAutoPowerOffCount);
    LIB_FUNCTION("MbTt1EHYCTg", "libScePad", 1, "libScePad", 1, 1, scePadSetButtonRemappingInfo);
    LIB_FUNCTION("MLA06oNfF+4", "libScePad", 1, "libScePad", 1, 1, scePadSetConnection);
    LIB_FUNCTION("bsbHFI0bl5s", "libScePad", 1, "libScePad", 1, 1, scePadSetExtensionReport);
    LIB_FUNCTION("xqgVCEflEDY", "libScePad", 1, "libScePad", 1, 1, scePadSetFeatureReport);
    LIB_FUNCTION("lrjFx4xWnY8", "libScePad", 1, "libScePad", 1, 1, scePadSetForceIntercepted);
    LIB_FUNCTION("RR4novUEENY", "libScePad", 1, "libScePad", 1, 1, scePadSetLightBar);
    LIB_FUNCTION("dhQXEvmrVNQ", "libScePad", 1, "libScePad", 1, 1, scePadSetLightBarBaseBrightness);
    LIB_FUNCTION("etaQhgPHDRY", "libScePad", 1, "libScePad", 1, 1, scePadSetLightBarBlinking);
    LIB_FUNCTION("iHuOWdvQVpg", "libScePad", 1, "libScePad", 1, 1, scePadSetLightBarForTracker);
    LIB_FUNCTION("o-6Y99a8dKU", "libScePad", 1, "libScePad", 1, 1, scePadSetLoginUserNumber);
    LIB_FUNCTION("clVvL4ZDntw", "libScePad", 1, "libScePad", 1, 1, scePadSetMotionSensorState);
    LIB_FUNCTION("flYYxek1wy8", "libScePad", 1, "libScePad", 1, 1, scePadSetProcessFocus);
    LIB_FUNCTION("DmBx8K+jDWw", "libScePad", 1, "libScePad", 1, 1, scePadSetProcessPrivilege);
    LIB_FUNCTION("FbxEpTRDou8", "libScePad", 1, "libScePad", 1, 1,
                 scePadSetProcessPrivilegeOfButtonRemapping);
    LIB_FUNCTION("yah8Bk4TcYY", "libScePad", 1, "libScePad", 1, 1,
                 scePadSetShareButtonMaskForRemotePlay);
    LIB_FUNCTION("vDLMoJLde8I", "libScePad", 1, "libScePad", 1, 1, scePadSetTiltCorrectionState);
    LIB_FUNCTION("z+GEemoTxOo", "libScePad", 1, "libScePad", 1, 1, scePadSetUserColor);
    LIB_FUNCTION("yFVnOdGxvZY", "libScePad", 1, "libScePad", 1, 1, scePadSetVibration);
    LIB_FUNCTION("8BOObG94-tc", "libScePad", 1, "libScePad", 1, 1, scePadSetVibrationForce);
    LIB_FUNCTION("--jrY4SHfm8", "libScePad", 1, "libScePad", 1, 1, scePadSetVrTrackingMode);
    LIB_FUNCTION("zFJ35q3RVnY", "libScePad", 1, "libScePad", 1, 1, scePadShareOutputData);
    LIB_FUNCTION("80XdmVYsNPA", "libScePad", 1, "libScePad", 1, 1, scePadStartRecording);
    LIB_FUNCTION("gAHvg6JPIic", "libScePad", 1, "libScePad", 1, 1, scePadStopRecording);
    LIB_FUNCTION("Oi7FzRWFr0Y", "libScePad", 1, "libScePad", 1, 1, scePadSwitchConnection);
    LIB_FUNCTION("0MB5x-ieRGI", "libScePad", 1, "libScePad", 1, 1, scePadVertualDeviceAddDevice);
    LIB_FUNCTION("N7tpsjWQ87s", "libScePad", 1, "libScePad", 1, 1, scePadVirtualDeviceAddDevice);
    LIB_FUNCTION("PFec14-UhEQ", "libScePad", 1, "libScePad", 1, 1, scePadVirtualDeviceDeleteDevice);
    LIB_FUNCTION("pjPCronWdxI", "libScePad", 1, "libScePad", 1, 1,
                 scePadVirtualDeviceDisableButtonRemapping);
    LIB_FUNCTION("LKXfw7VJYqg", "libScePad", 1, "libScePad", 1, 1,
                 scePadVirtualDeviceGetRemoteSetting);
    LIB_FUNCTION("IWOyO5jKuZg", "libScePad", 1, "libScePad", 1, 1, scePadVirtualDeviceInsertData);
    LIB_FUNCTION("KLmYx9ij2h0", "libScePad", 1, "libScePad", 1, 1, Func_28B998C7D8A3DA1D);
    LIB_FUNCTION("KY0hSB+Uyfo", "libScePad", 1, "libScePad", 1, 1, Func_298D21481F94C9FA);
    LIB_FUNCTION("UeUUvNOgXKU", "libScePad", 1, "libScePad", 1, 1, Func_51E514BCD3A05CA5);
    LIB_FUNCTION("ickjfjk9okM", "libScePad", 1, "libScePad", 1, 1, Func_89C9237E393DA243);
    LIB_FUNCTION("7xA+hFtvBCA", "libScePad", 1, "libScePad", 1, 1, Func_EF103E845B6F0420);
};

} // namespace Libraries::Pad
