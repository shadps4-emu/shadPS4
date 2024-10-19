// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/config.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "input/controller.h"
#include "pad.h"

namespace Libraries::Pad {

static bool g_initialized = false;

static bool g_pad_standard_connected = false;
static bool g_pad_standard_special_connected = false;

constexpr auto PAD_STANDARD_HANDLER = 0xBC1;
constexpr auto PAD_SPECIAL_HANDLER = 0xBC2;

void OrbisPadData::CopyFromState(const Input::State& state) {
    buttons = state.buttonsState;
    leftStick.x = state.axes[static_cast<int>(Input::Axis::LeftX)];
    leftStick.y = state.axes[static_cast<int>(Input::Axis::LeftY)];
    rightStick.x = state.axes[static_cast<int>(Input::Axis::RightX)];
    rightStick.y = state.axes[static_cast<int>(Input::Axis::RightY)];
    analogButtons.l2 = state.axes[static_cast<int>(Input::Axis::TriggerLeft)];
    analogButtons.r2 = state.axes[static_cast<int>(Input::Axis::TriggerRight)];
    orientation.x = 0.0f;
    orientation.y = 0.0f;
    orientation.z = 0.0f;
    orientation.w = 1.0f;
    acceleration.x = 0.0f;
    acceleration.y = 0.0f;
    acceleration.z = 0.0f;
    angularVelocity.x = 0.0f;
    angularVelocity.y = 0.0f;
    angularVelocity.z = 0.0f;
    touchData.touchNum = (state.touchpad[0].state ? 1 : 0) + (state.touchpad[1].state ? 1 : 0);
    touchData.touch[0].x = state.touchpad[0].x;
    touchData.touch[0].y = state.touchpad[0].y;
    touchData.touch[0].id = 1;
    touchData.touch[1].x = state.touchpad[1].x;
    touchData.touch[1].y = state.touchpad[1].y;
    touchData.touch[1].id = 2;
    connected = true;
    timestamp = state.time;
    connectedCount = 1;
    deviceUniqueDataLen = 0;
}

int PS4_SYSV_ABI scePadClose(s32 handle) {
    LOG_DEBUG(Lib_Pad, "called handle = {}", handle);
    if (!g_initialized) {
        return ORBIS_PAD_ERROR_NOT_INITIALIZED;
    }
    if (handle == PAD_STANDARD_HANDLER) {
        if (!g_pad_standard_connected) {
            return ORBIS_PAD_ERROR_INVALID_HANDLE;
        }
        g_pad_standard_connected = false;
    }
    if (handle == PAD_SPECIAL_HANDLER) {
        if (!g_pad_standard_special_connected) {
            return ORBIS_PAD_ERROR_INVALID_HANDLE;
        }
        g_pad_standard_special_connected = false;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadConnectPort() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadDeviceClassGetExtendedInformation(
    s32 handle, OrbisPadDeviceClassExtendedInformation* pExtInfo) {
    LOG_DEBUG(Lib_Pad, "called handle = {}", handle);

    if (!g_initialized) {
        return ORBIS_PAD_ERROR_NOT_INITIALIZED;
    }

    if (pExtInfo == nullptr) {
        return ORBIS_PAD_ERROR_INVALID_ARG;
    }

    if (handle == PAD_STANDARD_HANDLER) {
        if (!g_pad_standard_connected) {
            return ORBIS_PAD_ERROR_INVALID_HANDLE;
        }
        pExtInfo->deviceClass = ORBIS_PAD_DEVICE_CLASS_STANDARD;
        return ORBIS_OK;
    }
    if (handle == PAD_SPECIAL_HANDLER) {
        if (!g_pad_standard_special_connected) {
            return ORBIS_PAD_ERROR_INVALID_HANDLE;
        }
        pExtInfo->deviceClass = (OrbisPadDeviceClass)Config::getSpecialPadClass();
        switch (pExtInfo->deviceClass) {
        case ORBIS_PAD_DEVICE_CLASS_STEERING_WHEEL: {
            auto& data = pExtInfo->classData.steeringWheel;
            data.maxPhysicalWheelAngle = 360;
            data.capability = 0b1110; // Handbrake, Shift, 3 pedals
        } break;
        default:
            memset(pExtInfo->classData.data, 0, sizeof(pExtInfo->classData.data));
            break;
        }
        return ORBIS_OK;
    }

    return ORBIS_PAD_ERROR_INVALID_HANDLE;
}

int PS4_SYSV_ABI scePadDeviceClassParseData(s32 handle, const OrbisPadData* pData,
                                            OrbisPadDeviceClassData* pDeviceClassData) {
    LOG_TRACE(Lib_Pad, "called handle = {}", handle);
    if (!g_initialized) {
        return ORBIS_PAD_ERROR_NOT_INITIALIZED;
    }
    if (pData == nullptr || pDeviceClassData == nullptr) {
        return ORBIS_PAD_ERROR_INVALID_ARG;
    }

    if (handle == PAD_STANDARD_HANDLER) {
        if (!g_pad_standard_connected) {
            return ORBIS_PAD_ERROR_INVALID_HANDLE;
        }
        pDeviceClassData->deviceClass = ORBIS_PAD_DEVICE_CLASS_STANDARD;
        return ORBIS_OK;
    }

    if (handle == PAD_SPECIAL_HANDLER) {
        if (!g_pad_standard_special_connected) {
            return ORBIS_PAD_ERROR_INVALID_HANDLE;
        }
        auto pad_class = (OrbisPadDeviceClass)Config::getSpecialPadClass();
        pDeviceClassData->deviceClass = pad_class;
        switch (pad_class) {
        case ORBIS_PAD_DEVICE_CLASS_GUITAR: {
            LOG_ERROR(Lib_Pad, "(STUBBED) guitar not implemented");
            auto& data = pDeviceClassData->classData.guitar;
            // TODO implement guitar
        } break;
        case ORBIS_PAD_DEVICE_CLASS_DRUM: {
            LOG_ERROR(Lib_Pad, "(STUBBED) drum not implemented");
            auto& data = pDeviceClassData->classData.drum;
            // TODO implement drum
        } break;
        case ORBIS_PAD_DEVICE_CLASS_STEERING_WHEEL: {
            auto& data = pDeviceClassData->classData.steeringWheel;
            // TODO proper implement steering wheel
            auto& left_stick = pData->leftStick.x;
            data.steeringWheelAngle = static_cast<float>(left_stick - 0x7F) / 127.0f * 180.0f;
            if (data.steeringWheelAngle == 0.0) {
                data.steeringWheel = 0x80;
            } else {
                data.steeringWheel =
                    static_cast<u16>((data.steeringWheelAngle / 360.0f + 0.5f) * 0xFFFF);
            }
            data.acceleratorPedal = static_cast<u16>(pData->analogButtons.r2) * 0x102;
            data.brakePedal = static_cast<u16>(pData->analogButtons.l2) * 0x102;
            data.clutchPedal = pData->buttons & ORBIS_PAD_BUTTON_L1 ? 0xFFFF : 0x0000;
            data.handBrake = pData->buttons & ORBIS_PAD_BUTTON_R1 ? 0xFFFF : 0x0000;

            static int gear = 1;
            static bool switch_gear_up_pressed_last = false;
            static bool switch_gear_down_pressed_last = false;
            bool switch_gear_up_pressed = pData->buttons & ORBIS_PAD_BUTTON_SQUARE;
            bool switch_gear_down_pressed = pData->buttons & ORBIS_PAD_BUTTON_CROSS;
            if (switch_gear_up_pressed != switch_gear_up_pressed_last) {
                switch_gear_up_pressed_last = switch_gear_up_pressed;
                if (switch_gear_up_pressed) {
                    if (gear < 7) {
                        ++gear;
                    }
                }
            }
            if (switch_gear_down_pressed != switch_gear_down_pressed_last) {
                switch_gear_down_pressed_last = switch_gear_down_pressed;
                if (switch_gear_down_pressed) {
                    if (gear > 0) {
                        --gear;
                    }
                }
            }

            if (gear == 0) {
                data.gear = 1 << 7;
            } else {
                data.gear = 1 << (gear - 1);
            }
        } break;
        case ORBIS_PAD_DEVICE_CLASS_FLIGHT_STICK: {
            LOG_ERROR(Lib_Pad, "(STUBBED) flight stick not implemented");
            auto& data = pDeviceClassData->classData.flightStick;
            // TODO implement flight stick
        } break;
        default:
            pDeviceClassData->bDataValid = false;
            break;
        }
        return ORBIS_OK;
    }

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

    if (!g_initialized) {
        return ORBIS_PAD_ERROR_NOT_INITIALIZED;
    }

    if (handle == PAD_STANDARD_HANDLER) {
        if (!g_pad_standard_connected) {
            return ORBIS_PAD_ERROR_INVALID_HANDLE;
        }
    } else if (handle == PAD_SPECIAL_HANDLER) {
        if (!g_pad_standard_special_connected) {
            return ORBIS_PAD_ERROR_INVALID_HANDLE;
        }
    } else {
        return ORBIS_PAD_ERROR_INVALID_HANDLE;
    }

    pInfo->touchPadInfo.pixelDensity = 1;
    pInfo->touchPadInfo.resolution.x = 1920;
    pInfo->touchPadInfo.resolution.y = 950;
    pInfo->stickInfo.deadZoneLeft = 2;
    pInfo->stickInfo.deadZoneRight = 2;
    pInfo->connectionType = 0; // Local connection
    pInfo->connectedCount = 1;
    pInfo->connected = true;
    if (handle == PAD_STANDARD_HANDLER) {
        pInfo->deviceClass = ORBIS_PAD_DEVICE_CLASS_STANDARD;
    } else if (handle == PAD_SPECIAL_HANDLER) {
        pInfo->deviceClass = (OrbisPadDeviceClass)Config::getSpecialPadClass();
    } else {
        UNREACHABLE();
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
    LOG_DEBUG(Lib_Pad, "called handle = {}", handle);

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
    LOG_DEBUG(Lib_Pad, "called");
    g_initialized = true;
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
    LOG_DEBUG(Lib_Pad, "Called user_id = {} type = {} index = {}", userId,
              type == ORBIS_PAD_PORT_TYPE_STANDARD  ? "standard"
              : type == ORBIS_PAD_PORT_TYPE_SPECIAL ? "special"
                                                    : "unknown",
              index);
    if (!g_initialized) {
        return ORBIS_PAD_ERROR_NOT_INITIALIZED;
    }
    if (type == ORBIS_PAD_PORT_TYPE_STANDARD) {
        if (g_pad_standard_connected) {
            return ORBIS_PAD_ERROR_ALREADY_OPENED;
        }
        g_pad_standard_connected = true;
        return PAD_STANDARD_HANDLER;
    }

    if (type == ORBIS_PAD_PORT_TYPE_SPECIAL) {
        if (!Config::getUseSpecialPad()) {
            return ORBIS_PAD_ERROR_DEVICE_NOT_CONNECTED;
        }
        if (g_pad_standard_special_connected) {
            return ORBIS_PAD_ERROR_ALREADY_OPENED;
        }
        g_pad_standard_special_connected = true;
        return PAD_SPECIAL_HANDLER;
    }

    return ORBIS_PAD_ERROR_INVALID_ARG;
}

int PS4_SYSV_ABI scePadOpenExt(s32 userId, s32 type, s32 index,
                               const OrbisPadOpenExtParam* pParam) {
    LOG_DEBUG(Lib_Pad, "Redirecting call to scePadOpen");
    return scePadOpen(userId, type, index, nullptr);
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
    LOG_TRACE(Lib_Pad, "called handle = {} num = {}", handle, num);

    if (!g_initialized) {
        return ORBIS_PAD_ERROR_NOT_INITIALIZED;
    }

    if (num < 1 || num > 64) {
        return ORBIS_PAD_ERROR_INVALID_ARG;
    }

    // Hack to copy state between pads
    static bool connected = false;
    static std::array<Input::State, 64> states;
    static int state_count = 0;
    static bool has_std_data = false;
    static bool has_special_data = false;

    if (handle == PAD_STANDARD_HANDLER) {
        if (!g_pad_standard_connected) {
            return ORBIS_PAD_ERROR_INVALID_HANDLE;
        }

        if (!has_special_data) {
            int connected_count = 0;
            auto* controller = Common::Singleton<Input::GameController>::Instance();
            state_count = controller->ReadStates(states.data(), num, &connected, &connected_count);
            has_std_data = true;
        } else {
            has_special_data = false;
        }

        if (!connected) {
            pData[0] = OrbisPadData{
                .connected = false,
            };
            return 1;
        }

        for (int i = 0; i < state_count; i++) {
            pData[i].CopyFromState(states[i]);
        }

        return state_count;
    }

    if (handle == PAD_SPECIAL_HANDLER) {
        if (!g_pad_standard_special_connected) {
            return ORBIS_PAD_ERROR_INVALID_HANDLE;
        }

        if (!has_std_data) {
            int connected_count = 0;
            auto* controller = Common::Singleton<Input::GameController>::Instance();
            state_count = controller->ReadStates(states.data(), num, &connected, &connected_count);
            has_special_data = true;
        } else {
            has_std_data = false;
        }

        if (!connected) {
            pData[0] = OrbisPadData{
                .connected = false,
            };
            return 1;
        }

        for (int i = 0; i < state_count; i++) {
            pData[i].CopyFromState(states[i]);
        }

        return state_count;
    }

    return ORBIS_PAD_ERROR_INVALID_HANDLE;
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
    LOG_TRACE(Lib_Pad, "called handle = {}", handle);

    if (!g_initialized) {
        return ORBIS_PAD_ERROR_NOT_INITIALIZED;
    }

    if (handle == PAD_STANDARD_HANDLER) {
        if (!g_pad_standard_connected) {
            return ORBIS_PAD_ERROR_INVALID_HANDLE;
        }

    standard_handler:
        auto* controller = Common::Singleton<Input::GameController>::Instance();
        int connectedCount = 0;
        bool isConnected = false;
        Input::State state;
        controller->ReadState(&state, &isConnected, &connectedCount);

        pData->CopyFromState(state);

        return ORBIS_OK;
    }
    if (handle == PAD_SPECIAL_HANDLER) {
        if (!g_pad_standard_special_connected) {
            return ORBIS_PAD_ERROR_INVALID_HANDLE;
        }
        // TODO implement special pad
        goto standard_handler;
    }
    return ORBIS_PAD_ERROR_INVALID_HANDLE;
}

int PS4_SYSV_ABI scePadReadStateExt() {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI scePadResetLightBar(s32 handle) {
    LOG_ERROR(Lib_Pad, "(STUBBED) called");
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
    if (pParam != nullptr) {
        LOG_INFO(Lib_Pad, "scePadSetLightBar called handle = {} rgb = {} {} {}", handle, pParam->r,
                 pParam->g, pParam->b);

        if (pParam->r < 0xD && pParam->g < 0xD && pParam->b < 0xD) {
            LOG_INFO(Lib_Pad, "Invalid lightbar setting");
            return ORBIS_PAD_ERROR_INVALID_LIGHTBAR_SETTING;
        }

        auto* controller = Common::Singleton<Input::GameController>::Instance();
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
        auto* controller = Common::Singleton<Input::GameController>::Instance();
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

void RegisterlibScePad(Core::Loader::SymbolsResolver* sym) {
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
