// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/hmd/hmd.h"
#include "core/libraries/hmd/hmd_error.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"

namespace Libraries::Hmd {

static bool g_library_initialized = false;
static s32 g_firmware_version = 0;
static s32 g_internal_handle = 0;
static Libraries::UserService::OrbisUserServiceUserId g_user_id = -1;

s32 PS4_SYSV_ABI sceHmdInitialize(const OrbisHmdInitializeParam* param) {
    if (g_library_initialized) {
        return ORBIS_HMD_ERROR_ALREADY_INITIALIZED;
    }
    if (param == nullptr) {
        return ORBIS_HMD_ERROR_PARAMETER_NULL;
    }
    LOG_WARNING(Lib_Hmd, "PSVR headsets are not supported yet");
    if (param->reserved0 != nullptr) {
        sceHmdDistortionInitialize(param->reserved0);
    }
    g_library_initialized = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInitialize315(const OrbisHmdInitializeParam* param) {
    if (g_library_initialized) {
        return ORBIS_HMD_ERROR_ALREADY_INITIALIZED;
    }
    if (param == nullptr) {
        return ORBIS_HMD_ERROR_PARAMETER_NULL;
    }
    LOG_WARNING(Lib_Hmd, "PSVR headsets are not supported yet");
    g_library_initialized = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdOpen(Libraries::UserService::OrbisUserServiceUserId user_id, s32 type,
                            s32 index, OrbisHmdOpenParam* param) {
    LOG_DEBUG(Lib_Hmd, "called");
    if (!g_library_initialized) {
        return ORBIS_HMD_ERROR_NOT_INITIALIZED;
    }
    if (type != 0 || index != 0 || param != nullptr) {
        return ORBIS_HMD_ERROR_PARAMETER_INVALID;
    }
    if (g_internal_handle != 0) {
        return ORBIS_HMD_ERROR_ALREADY_OPENED;
    }
    if (user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID ||
        user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_SYSTEM) {
        return ORBIS_HMD_ERROR_PARAMETER_INVALID;
    }

    // Return positive value representing handle.
    // Internal libSceVrTracker logic requires this handle to be different from other devices.
    g_user_id = user_id;
    g_internal_handle = 0xf000000;
    return g_internal_handle;
}

s32 PS4_SYSV_ABI sceHmdGet2DEyeOffset(s32 handle, OrbisHmdEyeOffset* left_offset,
                                      OrbisHmdEyeOffset* right_offset) {
    LOG_DEBUG(Lib_Hmd, "called");
    if (!g_library_initialized) {
        return ORBIS_HMD_ERROR_NOT_INITIALIZED;
    }
    if (handle != g_internal_handle) {
        return ORBIS_HMD_ERROR_HANDLE_INVALID;
    }
    if (g_firmware_version >= Common::ElfInfo::FW_45) {
        // Due to some faulty in-library checks, a missing headset results in this error
        // instead of the expected ORBIS_HMD_ERROR_DEVICE_DISCONNECTED error.
        return ORBIS_HMD_ERROR_HANDLE_INVALID;
    }
    if (left_offset == nullptr || right_offset == nullptr) {
        return ORBIS_HMD_ERROR_PARAMETER_NULL;
    }

    // Return default values
    left_offset->offset_x = -0.0315;
    left_offset->offset_y = 0;
    left_offset->offset_z = 0;
    right_offset->offset_x = 0.0315;
    right_offset->offset_y = 0;
    right_offset->offset_z = 0;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdGetAssyError(void* data) {
    LOG_DEBUG(Lib_Hmd, "called");
    if (data == nullptr) {
        return ORBIS_HMD_ERROR_PARAMETER_NULL;
    }
    if (!g_library_initialized) {
        return ORBIS_HMD_ERROR_NOT_INITIALIZED;
    }

    return ORBIS_HMD_ERROR_DEVICE_DISCONNECTED;
}

s32 PS4_SYSV_ABI sceHmdGetDeviceInformation(OrbisHmdDeviceInformation* info) {
    LOG_DEBUG(Lib_Hmd, "called");
    if (info == nullptr) {
        return ORBIS_HMD_ERROR_PARAMETER_NULL;
    }
    if (!g_library_initialized) {
        return ORBIS_HMD_ERROR_NOT_INITIALIZED;
    }

    memset(info, 0, sizeof(OrbisHmdDeviceInformation));
    info->status = OrbisHmdDeviceStatus::ORBIS_HMD_DEVICE_STATUS_NOT_DETECTED;
    info->user_id = g_user_id;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdGetDeviceInformationByHandle(s32 handle, OrbisHmdDeviceInformation* info) {
    LOG_DEBUG(Lib_Hmd, "called");
    if (handle != g_internal_handle) {
        return ORBIS_HMD_ERROR_HANDLE_INVALID;
    }
    if (g_firmware_version >= Common::ElfInfo::FW_45) {
        // Due to some faulty in-library checks, a missing headset results in this error
        // instead of the expected ORBIS_HMD_ERROR_DEVICE_DISCONNECTED error.
        return ORBIS_HMD_ERROR_HANDLE_INVALID;
    }
    if (info == nullptr) {
        return ORBIS_HMD_ERROR_PARAMETER_NULL;
    }
    if (!g_library_initialized) {
        return ORBIS_HMD_ERROR_NOT_INITIALIZED;
    }

    memset(info, 0, sizeof(OrbisHmdDeviceInformation));
    info->status = OrbisHmdDeviceStatus::ORBIS_HMD_DEVICE_STATUS_NOT_DETECTED;
    info->user_id = g_user_id;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdGetFieldOfView(s32 handle, OrbisHmdFieldOfView* field_of_view) {
    LOG_DEBUG(Lib_Hmd, "called");
    if (field_of_view == nullptr) {
        return ORBIS_HMD_ERROR_PARAMETER_NULL;
    }
    if (handle != g_internal_handle) {
        return ORBIS_HMD_ERROR_HANDLE_INVALID;
    }
    if (g_firmware_version >= Common::ElfInfo::FW_45) {
        // Due to some faulty in-library checks, a missing headset results in this error
        // instead of the expected ORBIS_HMD_ERROR_DEVICE_DISCONNECTED error.
        return ORBIS_HMD_ERROR_HANDLE_INVALID;
    }
    if (!g_library_initialized) {
        return ORBIS_HMD_ERROR_NOT_INITIALIZED;
    }

    // These values are a hardcoded return when a headset is connected.
    // Leaving this here for future developers.
    // field_of_view->tan_out = 1.20743;
    // field_of_view->tan_in = 1.181346;
    // field_of_view->tan_top = 1.262872;
    // field_of_view->tan_bottom = 1.262872;

    // Fails internally due to some internal library checks that break without a connected headset.
    return ORBIS_HMD_ERROR_HANDLE_INVALID;
}

s32 PS4_SYSV_ABI sceHmdGetInertialSensorData(s32 handle, void* data, s32 unk) {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    if (!g_library_initialized) {
        return ORBIS_HMD_ERROR_NOT_INITIALIZED;
    }
    if (handle != g_internal_handle) {
        return ORBIS_HMD_ERROR_HANDLE_INVALID;
    }
    if (g_firmware_version >= Common::ElfInfo::FW_45) {
        // Due to some faulty in-library checks, a missing headset results in this error
        // instead of the expected ORBIS_HMD_ERROR_DEVICE_DISCONNECTED error.
        return ORBIS_HMD_ERROR_HANDLE_INVALID;
    }

    return ORBIS_HMD_ERROR_DEVICE_DISCONNECTED;
}

s32 PS4_SYSV_ABI sceHmdClose(s32 handle) {
    LOG_DEBUG(Lib_Hmd, "called");
    if (!g_library_initialized) {
        return ORBIS_HMD_ERROR_NOT_INITIALIZED;
    }
    if (handle != g_internal_handle) {
        return ORBIS_HMD_ERROR_HANDLE_INVALID;
    }

    g_internal_handle = 0;
    g_user_id = -1;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdTerminate() {
    LOG_DEBUG(Lib_Hmd, "called");
    if (!g_library_initialized) {
        return ORBIS_HMD_ERROR_NOT_INITIALIZED;
    }
    sceHmdDistortionTerminate();
    g_library_initialized = false;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternal3dAudioClose() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternal3dAudioOpen() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternal3dAudioSendData() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalAnotherScreenClose() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalAnotherScreenGetAudioStatus() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalAnotherScreenGetFadeState() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalAnotherScreenGetVideoStatus() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalAnotherScreenOpen() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalAnotherScreenSendAudio() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalAnotherScreenSendVideo() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalAnotherScreenSetFadeAndSwitch() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalBindDeviceWithUserId() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalCheckDeviceModelMk3() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalCheckS3dPassModeAvailable() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalCrashReportCancel() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalCrashReportClose() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalCrashReportOpen() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalCrashReportReadData() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalCrashReportReadDataSize() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalCreateSharedMemory() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalDfuCheckAfterPvt() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalDfuCheckPartialUpdateAvailable() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalDfuClose() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalDfuGetStatus() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalDfuOpen() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalDfuReset() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalDfuSend() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalDfuSendSize() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalDfuSetMode() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalDfuStart() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalEventInitialize() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetBrightness() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetCrashDumpInfo() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetDebugMode() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetDebugSocialScreenMode() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetDebugTextMode() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetDefaultLedData() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetDemoMode() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetDeviceInformation() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetDeviceInformationByHandle() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetDeviceStatus(OrbisHmdDeviceStatus* status) {
    LOG_DEBUG(Lib_Hmd, "called");
    if (status == nullptr) {
        return ORBIS_HMD_ERROR_PARAMETER_NULL;
    }
    // Internal function fails with error DEVICE_DISCONNECTED
    *status = OrbisHmdDeviceStatus::ORBIS_HMD_DEVICE_STATUS_NOT_DETECTED;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetEyeStatus() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetHmuOpticalParam() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetHmuPowerStatusForDebug() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetHmuSerialNumber() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetIPD() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetIpdSettingEnableForSystemService() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetPuBuildNumber() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetPuPositionParam() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetPuRevision() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetPUSerialNumber() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetPUVersion() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetRequiredPUPVersion() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetStatusReport() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetTv4kCapability() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetVirtualDisplayDepth() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetVirtualDisplayHeight() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetVirtualDisplaySize() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalGetVr2dData() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalIsCommonDlgMiniAppVr2d() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalIsCommonDlgVr2d() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalIsGameVr2d() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalIsMiniAppVr2d() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalMapSharedMemory() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalMirroringModeSetAspect() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalMirroringModeSetAspectDebug() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalMmapGetCount() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalMmapGetModeId() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalMmapGetSensorCalibrationData() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalMmapIsConnect() {
    LOG_DEBUG(Lib_Hmd, "called");
    // Returns 0 when device is disconnected.
    return 0;
}

s32 PS4_SYSV_ABI sceHmdInternalPushVr2dData() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalRegisterEventCallback() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalResetInertialSensor() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalResetLedForVrTracker() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalResetLedForVsh() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSeparateModeClose() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSeparateModeGetAudioStatus() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSeparateModeGetVideoStatus() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSeparateModeOpen() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSeparateModeSendAudio() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSeparateModeSendVideo() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetBrightness() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetCrashReportCommand() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetDebugGpo() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetDebugMode() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetDebugSocialScreenMode() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetDebugTextMode() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetDefaultLedData() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetDemoMode() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetDeviceConnection() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetForcedCrash() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetHmuPowerControl() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetHmuPowerControlForDebug() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetIPD() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetIpdSettingEnableForSystemService() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetLedOn() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetM2LedBrightness() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetM2LedOn() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetPortConnection() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetPortStatus() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetS3dPassMode() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetSidetone() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetUserType() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetVirtualDisplayDepth() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetVirtualDisplayHeight() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetVirtualDisplaySize() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSetVRMode() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSocialScreenGetFadeState() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSocialScreenSetFadeAndSwitch() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdInternalSocialScreenSetOutput() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_202D0D1A687FCD2F() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_358DBF818A3D8A12() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_5CCBADA76FE8F40E() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_63D403167DC08CF0() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_69383B2B4E3AEABF(s32 handle, void* data, s32 unk) {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    if (!g_library_initialized) {
        return ORBIS_HMD_ERROR_NOT_INITIALIZED;
    }
    if (handle != g_internal_handle) {
        return ORBIS_HMD_ERROR_HANDLE_INVALID;
    }
    if (g_firmware_version >= Common::ElfInfo::FW_45) {
        // Due to some faulty in-library checks, a missing headset results in this error
        // instead of the expected ORBIS_HMD_ERROR_DEVICE_DISCONNECTED error.
        return ORBIS_HMD_ERROR_HANDLE_INVALID;
    }

    return ORBIS_HMD_ERROR_DEVICE_DISCONNECTED;
}

s32 PS4_SYSV_ABI Func_791560C32F4F6D68() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_7C955961EA85B6D3() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_9952277839236BA7() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_9A276E739E54EEAF() {
    // Stubbed on real hardware.
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_9E501994E289CBE7() {
    // Stubbed on real hardware.
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_A31F4DA8B3BD2E12() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_A92D7C23AC364993() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_ADCCC25CB876FDBE() {
    // Stubbed on real hardware.
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_B16652641FE69F0E() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_FC193BD653F2AF2E() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_FF2E0E53015FE231() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    Libraries::Kernel::sceKernelGetCompiledSdkVersion(&g_firmware_version);
    LIB_FUNCTION("6biw1XHTSqQ", "libSceHmd", 1, "libSceHmd", sceHmdClose);
    LIB_FUNCTION("BWY-qKM5hxE", "libSceHmd", 1, "libSceHmd", sceHmdGet2DEyeOffset);
    LIB_FUNCTION("Yx+CuF11D3Q", "libSceHmd", 1, "libSceHmd", sceHmdGetAssyError);
    LIB_FUNCTION("thDt9upZlp8", "libSceHmd", 1, "libSceHmd", sceHmdGetDeviceInformation);
    LIB_FUNCTION("1pxQfif1rkE", "libSceHmd", 1, "libSceHmd", sceHmdGetDeviceInformationByHandle);
    LIB_FUNCTION("NPQwYFqi0bs", "libSceHmd", 1, "libSceHmd", sceHmdGetFieldOfView);
    LIB_FUNCTION("rU3HK9Q0r8o", "libSceHmd", 1, "libSceHmd", sceHmdGetInertialSensorData);
    LIB_FUNCTION("K4KnH0QkT2c", "libSceHmd", 1, "libSceHmd", sceHmdInitialize);
    LIB_FUNCTION("s-J66ar9g50", "libSceHmd", 1, "libSceHmd", sceHmdInitialize315);
    LIB_FUNCTION("riPQfAdebHk", "libSceHmd", 1, "libSceHmd", sceHmdInternal3dAudioClose);
    LIB_FUNCTION("wHnZU1qtiqw", "libSceHmd", 1, "libSceHmd", sceHmdInternal3dAudioOpen);
    LIB_FUNCTION("NuEjeN8WCBA", "libSceHmd", 1, "libSceHmd", sceHmdInternal3dAudioSendData);
    LIB_FUNCTION("QasPTUPWVZE", "libSceHmd", 1, "libSceHmd", sceHmdInternalAnotherScreenClose);
    LIB_FUNCTION("Wr5KVtyVDG0", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalAnotherScreenGetAudioStatus);
    LIB_FUNCTION("whRxl6Hhrzg", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalAnotherScreenGetFadeState);
    LIB_FUNCTION("w8BEUsIYn8w", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalAnotherScreenGetVideoStatus);
    LIB_FUNCTION("0cQDAbkOt2A", "libSceHmd", 1, "libSceHmd", sceHmdInternalAnotherScreenOpen);
    LIB_FUNCTION("Asczi8gw1NM", "libSceHmd", 1, "libSceHmd", sceHmdInternalAnotherScreenSendAudio);
    LIB_FUNCTION("6+v7m1vwE+0", "libSceHmd", 1, "libSceHmd", sceHmdInternalAnotherScreenSendVideo);
    LIB_FUNCTION("E0BLvy57IiQ", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalAnotherScreenSetFadeAndSwitch);
    LIB_FUNCTION("UTqrWB+1+SU", "libSceHmd", 1, "libSceHmd", sceHmdInternalBindDeviceWithUserId);
    LIB_FUNCTION("ego1YdqNGpI", "libSceHmd", 1, "libSceHmd", sceHmdInternalCheckDeviceModelMk3);
    LIB_FUNCTION("WR7XsLdjcqQ", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalCheckS3dPassModeAvailable);
    LIB_FUNCTION("eMI1Hq+NEwY", "libSceHmd", 1, "libSceHmd", sceHmdInternalCrashReportCancel);
    LIB_FUNCTION("dI3StPLQlMM", "libSceHmd", 1, "libSceHmd", sceHmdInternalCrashReportClose);
    LIB_FUNCTION("lqPT-Bf1s4I", "libSceHmd", 1, "libSceHmd", sceHmdInternalCrashReportOpen);
    LIB_FUNCTION("QxhJs6zHUmU", "libSceHmd", 1, "libSceHmd", sceHmdInternalCrashReportReadData);
    LIB_FUNCTION("A2jWOLPzHHE", "libSceHmd", 1, "libSceHmd", sceHmdInternalCrashReportReadDataSize);
    LIB_FUNCTION("E9scVxt0DNg", "libSceHmd", 1, "libSceHmd", sceHmdInternalCreateSharedMemory);
    LIB_FUNCTION("6RclvsKxr3I", "libSceHmd", 1, "libSceHmd", sceHmdInternalDfuCheckAfterPvt);
    LIB_FUNCTION("cE99PJR6b8w", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalDfuCheckPartialUpdateAvailable);
    LIB_FUNCTION("SuE90Qscg0s", "libSceHmd", 1, "libSceHmd", sceHmdInternalDfuClose);
    LIB_FUNCTION("5f-6lp7L5cY", "libSceHmd", 1, "libSceHmd", sceHmdInternalDfuGetStatus);
    LIB_FUNCTION("dv2RqD7ZBd4", "libSceHmd", 1, "libSceHmd", sceHmdInternalDfuOpen);
    LIB_FUNCTION("pN0HjRU86Jo", "libSceHmd", 1, "libSceHmd", sceHmdInternalDfuReset);
    LIB_FUNCTION("mdc++HCXSsQ", "libSceHmd", 1, "libSceHmd", sceHmdInternalDfuSend);
    LIB_FUNCTION("gjyqnphjGZE", "libSceHmd", 1, "libSceHmd", sceHmdInternalDfuSendSize);
    LIB_FUNCTION("bl4MkWNLxKs", "libSceHmd", 1, "libSceHmd", sceHmdInternalDfuSetMode);
    LIB_FUNCTION("a1LmvXhZ6TM", "libSceHmd", 1, "libSceHmd", sceHmdInternalDfuStart);
    LIB_FUNCTION("+UzzSnc0z9A", "libSceHmd", 1, "libSceHmd", sceHmdInternalEventInitialize);
    LIB_FUNCTION("uQc9P8Hrr6U", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetBrightness);
    LIB_FUNCTION("nK1g+MwMV10", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetCrashDumpInfo);
    LIB_FUNCTION("L5WZgOTw41Y", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetDebugMode);
    LIB_FUNCTION("3w8SkMfCHY0", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalGetDebugSocialScreenMode);
    LIB_FUNCTION("1Xmb76MHXug", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetDebugTextMode);
    LIB_FUNCTION("S0ITgPRkfUg", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetDefaultLedData);
    LIB_FUNCTION("mxjolbeBa78", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetDemoMode);
    LIB_FUNCTION("RFIi20Wp9j0", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetDeviceInformation);
    LIB_FUNCTION("P04LQJQZ43Y", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalGetDeviceInformationByHandle);
    LIB_FUNCTION("PPCqsD8B5uM", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetDeviceStatus);
    LIB_FUNCTION("-u82z1UhOq4", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetEyeStatus);
    LIB_FUNCTION("iINSFzCIaB8", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetHmuOpticalParam);
    LIB_FUNCTION("Csuvq2MMXHU", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalGetHmuPowerStatusForDebug);
    LIB_FUNCTION("UhFPniZvm8U", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetHmuSerialNumber);
    LIB_FUNCTION("aTg7K0466r8", "libSceHmd", 1, "libSceHmd", Func_69383B2B4E3AEABF);
    LIB_FUNCTION("9exeDpk7JU8", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetIPD);
    LIB_FUNCTION("yNtYRsxZ6-A", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalGetIpdSettingEnableForSystemService);
    LIB_FUNCTION("EKn+IFVsz0M", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetPuBuildNumber);
    LIB_FUNCTION("AxQ6HtktYfQ", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetPuPositionParam);
    LIB_FUNCTION("ynKv9QCSbto", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetPuRevision);
    LIB_FUNCTION("3jcyx7XOm7A", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetPUSerialNumber);
    LIB_FUNCTION("+PDyXnclP5w", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetPUVersion);
    LIB_FUNCTION("67q17ERGBuw", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetRequiredPUPVersion);
    LIB_FUNCTION("uGyN1CkvwYU", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetStatusReport);
    LIB_FUNCTION("p9lSvZujLuo", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetTv4kCapability);
    LIB_FUNCTION("-Z+-9u98m9o", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetVirtualDisplayDepth);
    LIB_FUNCTION("df+b0FQnnVQ", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetVirtualDisplayHeight);
    LIB_FUNCTION("i6yROd9ygJs", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetVirtualDisplaySize);
    LIB_FUNCTION("Aajiktl6JXU", "libSceHmd", 1, "libSceHmd", sceHmdInternalGetVr2dData);
    LIB_FUNCTION("GwFVF2KkIT4", "libSceHmd", 1, "libSceHmd", sceHmdInternalIsCommonDlgMiniAppVr2d);
    LIB_FUNCTION("LWQpWHOSUvk", "libSceHmd", 1, "libSceHmd", sceHmdInternalIsCommonDlgVr2d);
    LIB_FUNCTION("YiIVBPLxmfE", "libSceHmd", 1, "libSceHmd", sceHmdInternalIsGameVr2d);
    LIB_FUNCTION("LMlWs+oKHTg", "libSceHmd", 1, "libSceHmd", sceHmdInternalIsMiniAppVr2d);
    LIB_FUNCTION("nBv4CKUGX0Y", "libSceHmd", 1, "libSceHmd", sceHmdInternalMapSharedMemory);
    LIB_FUNCTION("4hTD8I3CyAk", "libSceHmd", 1, "libSceHmd", sceHmdInternalMirroringModeSetAspect);
    LIB_FUNCTION("EJwPtSSZykY", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalMirroringModeSetAspectDebug);
    LIB_FUNCTION("r7f7M5q3snU", "libSceHmd", 1, "libSceHmd", sceHmdInternalMmapGetCount);
    LIB_FUNCTION("gCjTEtEsOOw", "libSceHmd", 1, "libSceHmd", sceHmdInternalMmapGetModeId);
    LIB_FUNCTION("HAr740Mt9Hs", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalMmapGetSensorCalibrationData);
    LIB_FUNCTION("1PNiQR-7L6k", "libSceHmd", 1, "libSceHmd", sceHmdInternalMmapIsConnect);
    LIB_FUNCTION("9-jaAXUNG-A", "libSceHmd", 1, "libSceHmd", sceHmdInternalPushVr2dData);
    LIB_FUNCTION("1gkbLH5+kxU", "libSceHmd", 1, "libSceHmd", sceHmdInternalRegisterEventCallback);
    LIB_FUNCTION("6kHBllapJas", "libSceHmd", 1, "libSceHmd", sceHmdInternalResetInertialSensor);
    LIB_FUNCTION("k1W6RPkd0mc", "libSceHmd", 1, "libSceHmd", sceHmdInternalResetLedForVrTracker);
    LIB_FUNCTION("dp1wu22jSGc", "libSceHmd", 1, "libSceHmd", sceHmdInternalResetLedForVsh);
    LIB_FUNCTION("d2TeoKeqM5U", "libSceHmd", 1, "libSceHmd", sceHmdInternalSeparateModeClose);
    LIB_FUNCTION("WxsnAsjPF7Q", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalSeparateModeGetAudioStatus);
    LIB_FUNCTION("eOOeG9SpEuc", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalSeparateModeGetVideoStatus);
    LIB_FUNCTION("gA4Xnn+NSGk", "libSceHmd", 1, "libSceHmd", sceHmdInternalSeparateModeOpen);
    LIB_FUNCTION("stQ7AsondmE", "libSceHmd", 1, "libSceHmd", sceHmdInternalSeparateModeSendAudio);
    LIB_FUNCTION("jfnS-OoDayM", "libSceHmd", 1, "libSceHmd", sceHmdInternalSeparateModeSendVideo);
    LIB_FUNCTION("roHN4ml+tB8", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetBrightness);
    LIB_FUNCTION("0z2qLqedQH0", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetCrashReportCommand);
    LIB_FUNCTION("xhx5rVZEpnw", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetDebugGpo);
    LIB_FUNCTION("e7laRxRGCHc", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetDebugMode);
    LIB_FUNCTION("CRyJ7Q-ap3g", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalSetDebugSocialScreenMode);
    LIB_FUNCTION("dG4XPW4juU4", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetDebugTextMode);
    LIB_FUNCTION("rAXmGoO-VmE", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetDefaultLedData);
    LIB_FUNCTION("lu9I7jnUvWQ", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetDemoMode);
    LIB_FUNCTION("hyATMTuQSoQ", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetDeviceConnection);
    LIB_FUNCTION("c4mSi64bXUw", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetForcedCrash);
    LIB_FUNCTION("U9kPT4g1mFE", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetHmuPowerControl);
    LIB_FUNCTION("dX-MVpXIPwQ", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalSetHmuPowerControlForDebug);
    LIB_FUNCTION("4KIjvAf8PCA", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetIPD);
    LIB_FUNCTION("NbxTfUKO184", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalSetIpdSettingEnableForSystemService);
    LIB_FUNCTION("NnRKjf+hxW4", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetLedOn);
    LIB_FUNCTION("4AP0X9qGhqw", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetM2LedBrightness);
    LIB_FUNCTION("Mzzz2HPWM+8", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetM2LedOn);
    LIB_FUNCTION("LkBkse9Pit0", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetPortConnection);
    LIB_FUNCTION("v243mvYg0Y0", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetPortStatus);
    LIB_FUNCTION("EwXvkZpo9Go", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetS3dPassMode);
    LIB_FUNCTION("g3DKNOy1tYw", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetSidetone);
    LIB_FUNCTION("mjMsl838XM8", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetUserType);
    LIB_FUNCTION("8IS0KLkDNQY", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetVirtualDisplayDepth);
    LIB_FUNCTION("afhK5KcJOJY", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetVirtualDisplayHeight);
    LIB_FUNCTION("+zPvzIiB+BU", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetVirtualDisplaySize);
    LIB_FUNCTION("9z8Lc64NF1c", "libSceHmd", 1, "libSceHmd", sceHmdInternalSetVRMode);
    LIB_FUNCTION("s5EqYh5kbwM", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalSocialScreenGetFadeState);
    LIB_FUNCTION("a1LMFZtK9b0", "libSceHmd", 1, "libSceHmd",
                 sceHmdInternalSocialScreenSetFadeAndSwitch);
    LIB_FUNCTION("-6FjKlMA+Yc", "libSceHmd", 1, "libSceHmd", sceHmdInternalSocialScreenSetOutput);
    LIB_FUNCTION("d2g5Ij7EUzo", "libSceHmd", 1, "libSceHmd", sceHmdOpen);
    LIB_FUNCTION("z-RMILqP6tE", "libSceHmd", 1, "libSceHmd", sceHmdTerminate);
    LIB_FUNCTION("IC0NGmh-zS8", "libSceHmd", 1, "libSceHmd", Func_202D0D1A687FCD2F);
    LIB_FUNCTION("NY2-gYo9ihI", "libSceHmd", 1, "libSceHmd", Func_358DBF818A3D8A12);
    LIB_FUNCTION("XMutp2-o9A4", "libSceHmd", 1, "libSceHmd", Func_5CCBADA76FE8F40E);
    LIB_FUNCTION("Y9QDFn3AjPA", "libSceHmd", 1, "libSceHmd", Func_63D403167DC08CF0);
    LIB_FUNCTION("eRVgwy9PbWg", "libSceHmd", 1, "libSceHmd", Func_791560C32F4F6D68);
    LIB_FUNCTION("fJVZYeqFttM", "libSceHmd", 1, "libSceHmd", Func_7C955961EA85B6D3);
    LIB_FUNCTION("mVIneDkja6c", "libSceHmd", 1, "libSceHmd", Func_9952277839236BA7);
    LIB_FUNCTION("miduc55U7q8", "libSceHmd", 1, "libSceHmd", Func_9A276E739E54EEAF);
    LIB_FUNCTION("nlAZlOKJy+c", "libSceHmd", 1, "libSceHmd", Func_9E501994E289CBE7);
    LIB_FUNCTION("ox9NqLO9LhI", "libSceHmd", 1, "libSceHmd", Func_A31F4DA8B3BD2E12);
    LIB_FUNCTION("qS18I6w2SZM", "libSceHmd", 1, "libSceHmd", Func_A92D7C23AC364993);
    LIB_FUNCTION("rczCXLh2-b4", "libSceHmd", 1, "libSceHmd", Func_ADCCC25CB876FDBE);
    LIB_FUNCTION("sWZSZB-mnw4", "libSceHmd", 1, "libSceHmd", Func_B16652641FE69F0E);
    LIB_FUNCTION("-Bk71lPyry4", "libSceHmd", 1, "libSceHmd", Func_FC193BD653F2AF2E);
    LIB_FUNCTION("-y4OUwFf4jE", "libSceHmd", 1, "libSceHmd", Func_FF2E0E53015FE231);

    RegisterDistortion(sym);
    RegisterReprojection(sym);
};

} // namespace Libraries::Hmd