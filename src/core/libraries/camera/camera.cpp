// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/camera/camera.h"
#include "core/libraries/camera/camera_error.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"

namespace Libraries::Camera {

static bool g_library_opened = false;
static s32 g_firmware_version = 0;
static s32 g_handles = 0;

s32 PS4_SYSV_ABI sceCameraAccGetData() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraAudioClose() {
    LOG_DEBUG(Lib_Camera, "called");
    // Returns ORBIS_OK while internally failing
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraAudioGetData(void* data) {
    LOG_DEBUG(Lib_Camera, "called");
    if (data == nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    // Returns ORBIS_OK while internally failing
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraAudioGetData2(void* data) {
    LOG_DEBUG(Lib_Camera, "called");
    if (data == nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    // Returns ORBIS_OK while internally failing
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraAudioOpen() {
    LOG_DEBUG(Lib_Camera, "called");
    // Returns error fatal as a side effect of not having a camera attached.
    return ORBIS_CAMERA_ERROR_FATAL;
}

s32 PS4_SYSV_ABI sceCameraAudioReset() {
    LOG_DEBUG(Lib_Camera, "called");
    // Returns ORBIS_OK while internally failing
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraChangeAppModuleState() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraClose(s32 handle) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    // Decrement handles on close.
    // If no handles remain, then the library itself is considered closed.
    if (--g_handles == 0) {
        g_library_opened = false;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraCloseByHandle(s32 handle) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    // Decrement handles on close.
    // If no handles remain, then the library itself is considered closed.
    if (--g_handles == 0) {
        g_library_opened = false;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraDeviceOpen() {
    // Stubbed on real hardware
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetAttribute(s32 handle, OrbisCameraAttribute* attribute) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || attribute == nullptr || attribute->sizeThis != sizeof(OrbisCameraAttribute) ||
        attribute->channel >= OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        attribute->channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    auto channel = attribute->channel;

    // Set default attributes
    memset(attribute, 0, sizeof(OrbisCameraAttribute));
    attribute->sizeThis = sizeof(OrbisCameraAttribute);
    attribute->channel = channel;
    attribute->exposureGain.exposure = 83;
    attribute->exposureGain.gain = 100;
    attribute->whiteBalance.gainRed = 768;
    attribute->whiteBalance.gainBlue = 768;
    attribute->whiteBalance.gainGreen = 512;
    attribute->gamma.value = 4;
    attribute->saturation = 64;
    attribute->contrast = 32;
    attribute->sharpness = 1;
    attribute->hue = 1;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetAutoExposureGain(s32 handle, OrbisCameraChannel channel, u32* enable,
                                              OrbisCameraAutoExposureGainTarget* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel >= OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || enable == nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (option != nullptr && (g_firmware_version < Common::ElfInfo::FW_30 ||
                              option->sizeThis != sizeof(OrbisCameraAutoExposureGainTarget))) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    *enable = 0;
    if (option != nullptr) {
        option->sizeThis = 0;
        option->target = OrbisCameraAecAgcTarget::ORBIS_CAMERA_ATTRIBUTE_AECAGC_TARGET_DEF;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetAutoWhiteBalance(s32 handle, OrbisCameraChannel channel, u32* enable,
                                              void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel >= OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || enable == nullptr ||
        option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    *enable = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetCalibData() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraGetCalibDataFromDevice() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraGetCalibrationData(const OrbisCameraGetCalibrationDataParameter* param,
                                             OrbisCameraCalibrationData* calibration_data) {
    LOG_DEBUG(Lib_Camera, "called");
    if (param == nullptr || calibration_data == nullptr ||
        param->size != sizeof(OrbisCameraGetCalibrationDataParameter) || param->format_type != 0 ||
        param->function_type <
            OrbisCameraCalibrationDataFunctionType::
                ORBIS_CAMERA_CALIBRATION_DATA_FUNCTION_TYPE_IMAGE_RECTIFICATION ||
        param->function_type >
            OrbisCameraCalibrationDataFunctionType::
                ORBIS_CAMERA_CALIBRATION_DATA_FUNCTION_TYPE_IMAGE_INVERSE_RECTIFICATION) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraGetConfig(s32 handle, OrbisCameraConfig* config) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || config == nullptr || config->sizeThis != sizeof(OrbisCameraConfig)) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    // Set default config
    config->configType = ORBIS_CAMERA_CONFIG_TYPE1;
    config->sizeThis = 0;

    OrbisCameraConfigExtention default_extension;
    default_extension.format.formatLevel0 = ORBIS_CAMERA_FORMAT_YUV422;
    default_extension.format.formatLevel1 = ORBIS_CAMERA_SCALE_FORMAT_Y8;
    default_extension.format.formatLevel2 = ORBIS_CAMERA_SCALE_FORMAT_Y8;
    default_extension.format.formatLevel3 = ORBIS_CAMERA_SCALE_FORMAT_Y8;
    default_extension.resolution = ORBIS_CAMERA_RESOLUTION_1280X800;
    default_extension.framerate = ORBIS_CAMERA_FRAMERATE_60;
    default_extension.width = 0;
    default_extension.height = 0;
    default_extension.reserved1 = 0;

    memcpy(&config->configExtention[0], &default_extension, sizeof(OrbisCameraConfigExtention));
    memcpy(&config->configExtention[1], &default_extension, sizeof(OrbisCameraConfigExtention));

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetContrast(s32 handle, OrbisCameraChannel channel, u32* contrast,
                                      void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel >= OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || contrast == nullptr ||
        option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    *contrast = 32;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetDefectivePixelCancellation(s32 handle, OrbisCameraChannel channel,
                                                        u32* enable, void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel >= OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || enable == nullptr ||
        option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    *enable = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetDeviceConfig(s32 handle, OrbisCameraConfig* config) {
    if (handle < 1 || config == nullptr || config->sizeThis != sizeof(OrbisCameraConfig)) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    memset(config, 0, sizeof(OrbisCameraConfig));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetDeviceConfigWithoutHandle(OrbisCameraConfig* config) {
    if (config == nullptr || config->sizeThis != sizeof(OrbisCameraConfig)) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    memset(config, 0, sizeof(OrbisCameraConfig));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetDeviceID() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetDeviceIDWithoutOpen() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetDeviceInfo(s32 reserved, OrbisCameraDeviceInfo* device_info) {
    LOG_DEBUG(Lib_Camera, "called");
    if (reserved != 0 || device_info == nullptr ||
        device_info->sizeThis != sizeof(OrbisCameraDeviceInfo) || device_info->infoRevision != 1) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_INIT;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraGetExposureGain(s32 handle, OrbisCameraChannel channel,
                                          OrbisCameraExposureGain* exposure_gain, void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel >= OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || exposure_gain == nullptr ||
        option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    // Return default parameters
    exposure_gain->exposureControl = 0;
    exposure_gain->exposure = 83;
    exposure_gain->gain = 100;
    exposure_gain->mode = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetFrameData(s32 handle, OrbisCameraFrameData* frame_data) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || frame_data == nullptr || frame_data->sizeThis > 584) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraGetGamma(s32 handle, OrbisCameraChannel channel, OrbisCameraGamma* gamma,
                                   void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel >= OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || gamma == nullptr ||
        option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    // Return default parameters
    memset(gamma, 0, sizeof(OrbisCameraGamma));
    gamma->value = 4;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetHue(s32 handle, OrbisCameraChannel channel, s32* hue, void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel >= OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || hue == nullptr ||
        option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    *hue = 1;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetLensCorrection(s32 handle, OrbisCameraChannel channel, u32* enable,
                                            void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel >= OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || enable == nullptr ||
        option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    *enable = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetMmapConnectedCount(u32* count) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }
    if (count == nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }

    *count = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetProductInfo(void* product_info) {
    LOG_DEBUG(Lib_Camera, "(STUBBED) called");
    if (product_info == nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_INIT;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraGetRegister() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetRegistryInfo(void* registry_info) {
    LOG_DEBUG(Lib_Camera, "(STUBBED) called");
    if (registry_info == nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_INIT;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraGetSaturation(s32 handle, OrbisCameraChannel channel, u32* saturation,
                                        void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel >= OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || saturation == nullptr ||
        option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    *saturation = 64;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetSharpness(s32 handle, OrbisCameraChannel channel, u32* sharpness,
                                       void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel >= OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || sharpness == nullptr ||
        option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    *sharpness = 1;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetVrCaptureInfo() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetWhiteBalance(s32 handle, OrbisCameraChannel channel,
                                          OrbisCameraWhiteBalance* white_balance, void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel >= OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || white_balance == nullptr ||
        option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    // Set default parameters
    white_balance->whiteBalanceControl = 0;
    white_balance->gainRed = 768;
    white_balance->gainBlue = 768;
    white_balance->gainGreen = 512;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraInitializeRegistryCalibData() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_INIT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraIsAttached(s32 index) {
    LOG_INFO(Lib_Camera, "called");
    if (index != 0) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    // 0 = disconnected, 1 = connected
    return 0;
}

s32 PS4_SYSV_ABI sceCameraIsConfigChangeDone() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraIsValidFrameData(s32 handle, OrbisCameraFrameData* frame_data) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || frame_data == nullptr || frame_data->sizeThis > 584) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraOpen(Libraries::UserService::OrbisUserServiceUserId user_id, s32 type,
                               s32 index, OrbisCameraOpenParameter* param) {
    if (user_id != Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_SYSTEM || type != 0 ||
        index != 0) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    LOG_WARNING(Lib_Camera, "Cameras are not supported yet");

    g_library_opened = true;
    return ++g_handles;
}

s32 PS4_SYSV_ABI sceCameraOpenByModuleId() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraRemoveAppModuleFocus() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetAppModuleFocus() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetAttribute(s32 handle, OrbisCameraAttribute* attribute) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || attribute == nullptr || attribute->sizeThis != sizeof(OrbisCameraAttribute) ||
        attribute->channel > OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        attribute->channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetAttributeInternal() {
    // Stubbed on real hardware
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetAutoExposureGain(s32 handle, OrbisCameraChannel channel, u32 enable,
                                              OrbisCameraAutoExposureGainTarget* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel > OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (option != nullptr) {
        if (g_firmware_version < Common::ElfInfo::FW_30 ||
            option->sizeThis != sizeof(OrbisCameraAutoExposureGainTarget)) {
            return ORBIS_CAMERA_ERROR_PARAM;
        }
        if (option->target % 2 == 1 || option->target < ORBIS_CAMERA_ATTRIBUTE_AECAGC_TARGET_DEF ||
            option->target > ORBIS_CAMERA_ATTRIBUTE_AECAGC_TARGET_2_0) {
            return ORBIS_CAMERA_ERROR_PARAM;
        }
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetAutoWhiteBalance(s32 handle, OrbisCameraChannel channel, u32 enable,
                                              void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel > OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetCalibData() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetConfig(s32 handle, OrbisCameraConfig* config) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || config == nullptr || config->sizeThis != sizeof(OrbisCameraConfig)) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetConfigInternal(s32 handle, OrbisCameraConfig* config) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || config == nullptr || config->sizeThis != sizeof(OrbisCameraConfig)) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetContrast(s32 handle, OrbisCameraChannel channel, u32 contrast,
                                      void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel > OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetDebugStop(u32 debug_stop_enable) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    if (debug_stop_enable > 1) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetDefectivePixelCancellation(s32 handle, OrbisCameraChannel channel,
                                                        u32 enable, void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel > OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || enable > 1 || option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetDefectivePixelCancellationInternal(s32 handle,
                                                                OrbisCameraChannel channel,
                                                                u32 enable, void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel > OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || enable > 2 || option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetExposureGain(s32 handle, OrbisCameraChannel channel,
                                          OrbisCameraExposureGain* exposure_gain, void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel > OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || exposure_gain != nullptr ||
        option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetForceActivate() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetGamma(s32 handle, OrbisCameraChannel channel, OrbisCameraGamma* gamma,
                                   void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel > OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || gamma != nullptr ||
        option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetHue(s32 handle, OrbisCameraChannel channel, s32 hue, void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel > OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetLensCorrection(s32 handle, OrbisCameraChannel channel, u32 enable,
                                            void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel > OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || enable > 1 || option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetLensCorrectionInternal(s32 handle, OrbisCameraChannel channel,
                                                    u32 enable, void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel > OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || enable > 2 || option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetProcessFocus() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetProcessFocusByHandle() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetRegister() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetSaturation(s32 handle, OrbisCameraChannel channel, u32 saturation,
                                        void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel > OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetSharpness(s32 handle, OrbisCameraChannel channel, u32 sharpness,
                                       void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel > OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (g_firmware_version >= Common::ElfInfo::FW_35 && sharpness > 10) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraSetTrackerMode() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetUacModeInternal() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetVideoSync(s32 handle, OrbisCameraVideoSyncParameter* video_sync) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    if (handle < 1 || video_sync == nullptr ||
        video_sync->sizeThis != sizeof(OrbisCameraVideoSyncParameter) ||
        video_sync->videoSyncMode > 1 || video_sync->pModeOption != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetVideoSyncInternal(s32 handle,
                                               OrbisCameraVideoSyncParameter* video_sync) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetWhiteBalance(s32 handle, OrbisCameraChannel channel,
                                          OrbisCameraWhiteBalance* white_balance, void* option) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || channel > OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_BOTH ||
        channel < OrbisCameraChannel::ORBIS_CAMERA_CHANNEL_0 || white_balance == nullptr ||
        option != nullptr) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
}

s32 PS4_SYSV_ABI sceCameraStart(s32 handle, OrbisCameraStartParameter* param) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || param == nullptr || param->sizeThis != sizeof(OrbisCameraStartParameter)) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }
    if (g_firmware_version >= Common::ElfInfo::FW_25 &&
        (param->formatLevel[0] >= 0xf || param->formatLevel[1] >= 0xf ||
         (param->formatLevel[0] | param->formatLevel[1]) == 0)) {
        return ORBIS_CAMERA_ERROR_FORMAT_UNKNOWN;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraStartByHandle(s32 handle, OrbisCameraStartParameter* param) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1 || param == nullptr || param->sizeThis != sizeof(OrbisCameraStartParameter)) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraStop(s32 handle) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraStopByHandle(s32 handle) {
    LOG_DEBUG(Lib_Camera, "called");
    if (handle < 1) {
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }

    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    Libraries::Kernel::sceKernelGetCompiledSdkVersion(&g_firmware_version);

    LIB_FUNCTION("QhjrPkRPUZQ", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraAccGetData);
    LIB_FUNCTION("UFonL7xopFM", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraAudioClose);
    LIB_FUNCTION("fkZE7Hup2ro", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraAudioGetData);
    LIB_FUNCTION("hftC5A1C8OQ", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraAudioGetData2);
    LIB_FUNCTION("DhqqFiBU+6g", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraAudioOpen);
    LIB_FUNCTION("wyU98EXAYxU", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraAudioReset);
    LIB_FUNCTION("Y0pCDajzkVQ", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraChangeAppModuleState);
    LIB_FUNCTION("OMS9LlcrvBo", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraClose);
    LIB_FUNCTION("ztqH5qNTpTk", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraCloseByHandle);
    LIB_FUNCTION("nBH6i2s4Glc", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraDeviceOpen);
    LIB_FUNCTION("0btIPD5hg5A", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetAttribute);
    LIB_FUNCTION("oEi6vM-3E2c", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetAutoExposureGain);
    LIB_FUNCTION("qTPRMh4eY60", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetAutoWhiteBalance);
    LIB_FUNCTION("hHA1frlMxYE", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetCalibData);
    LIB_FUNCTION("5Oie5RArfWs", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetCalibDataFromDevice);
    LIB_FUNCTION("RHYJ7GKOSMg", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetCalibrationData);
    LIB_FUNCTION("ZaqmGEtYuL0", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetConfig);
    LIB_FUNCTION("a5xFueMZIMs", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetContrast);
    LIB_FUNCTION("tslCukqFE+E", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetDefectivePixelCancellation);
    LIB_FUNCTION("DSOLCrc3Kh8", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetDeviceConfig);
    LIB_FUNCTION("n+rFeP1XXyM", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetDeviceConfigWithoutHandle);
    LIB_FUNCTION("jTJCdyv9GLU", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetDeviceID);
    LIB_FUNCTION("-H3UwGQvNZI", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetDeviceIDWithoutOpen);
    LIB_FUNCTION("WZpxnSAM-ds", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetDeviceInfo);
    LIB_FUNCTION("ObIste7hqdk", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetExposureGain);
    LIB_FUNCTION("mxgMmR+1Kr0", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetFrameData);
    LIB_FUNCTION("WVox2rwGuSc", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetGamma);
    LIB_FUNCTION("zrIUDKZx0iE", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetHue);
    LIB_FUNCTION("XqYRHc4aw3w", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetLensCorrection);
    LIB_FUNCTION("B260o9pSzM8", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetMmapConnectedCount);
    LIB_FUNCTION("ULxbwqiYYuU", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetProductInfo);
    LIB_FUNCTION("olojYZKYiYs", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetRegister);
    LIB_FUNCTION("hawKak+Auw4", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetRegistryInfo);
    LIB_FUNCTION("RTDOsWWqdME", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetSaturation);
    LIB_FUNCTION("c6Fp9M1EXXc", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetSharpness);
    LIB_FUNCTION("IAz2HgZQWzE", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetVrCaptureInfo);
    LIB_FUNCTION("HX5524E5tMY", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetWhiteBalance);
    LIB_FUNCTION("0wnf2a60FqI", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraInitializeRegistryCalibData);
    LIB_FUNCTION("p6n3Npi3YY4", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraIsAttached);
    LIB_FUNCTION("wQfd7kfRZvo", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraIsConfigChangeDone);
    LIB_FUNCTION("U3BVwQl2R5Q", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraIsValidFrameData);
    LIB_FUNCTION("BHn83xrF92E", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraOpen);
    LIB_FUNCTION("eTywOSWsEiI", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraOpenByModuleId);
    LIB_FUNCTION("py8p6kZcHmA", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraRemoveAppModuleFocus);
    LIB_FUNCTION("j5isFVIlZLk", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetAppModuleFocus);
    LIB_FUNCTION("doPlf33ab-U", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetAttribute);
    LIB_FUNCTION("96F7zp1Xo+k", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetAttributeInternal);
    LIB_FUNCTION("yfSdswDaElo", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetAutoExposureGain);
    LIB_FUNCTION("zIKL4kZleuc", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetAutoWhiteBalance);
    LIB_FUNCTION("LEMk5cTHKEA", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetCalibData);
    LIB_FUNCTION("VQ+5kAqsE2Q", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetConfig);
    LIB_FUNCTION("9+SNhbctk64", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetConfigInternal);
    LIB_FUNCTION("3i5MEzrC1pg", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetContrast);
    LIB_FUNCTION("vejouEusC7g", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetDebugStop);
    LIB_FUNCTION("jMv40y2A23g", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetDefectivePixelCancellation);
    LIB_FUNCTION("vER3cIMBHqI", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetDefectivePixelCancellationInternal);
    LIB_FUNCTION("wgBMXJJA6K4", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetExposureGain);
    LIB_FUNCTION("jeTpU0MqKU0", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetForceActivate);
    LIB_FUNCTION("lhEIsHzB8r4", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetGamma);
    LIB_FUNCTION("QI8GVJUy2ZY", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetHue);
    LIB_FUNCTION("K7W7H4ZRwbc", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetLensCorrection);
    LIB_FUNCTION("eHa3vhGu2rQ", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetLensCorrectionInternal);
    LIB_FUNCTION("lS0tM6n+Q5E", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetProcessFocus);
    LIB_FUNCTION("NVITuK83Z7o", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetProcessFocusByHandle);
    LIB_FUNCTION("8MjO05qk5hA", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetRegister);
    LIB_FUNCTION("bSKEi2PzzXI", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetSaturation);
    LIB_FUNCTION("P-7MVfzvpsM", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetSharpness);
    LIB_FUNCTION("3VJOpzKoIeM", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetTrackerMode);
    LIB_FUNCTION("nnR7KAIDPv8", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetUacModeInternal);
    LIB_FUNCTION("wpeyFwJ+UEI", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetVideoSync);
    LIB_FUNCTION("8WtmqmE4edw", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetVideoSyncInternal);
    LIB_FUNCTION("k3zPIcgFNv0", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetWhiteBalance);
    LIB_FUNCTION("9EpRYMy7rHU", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraStart);
    LIB_FUNCTION("cLxF1QtHch0", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraStartByHandle);
    LIB_FUNCTION("2G2C0nmd++M", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraStop);
    LIB_FUNCTION("+X1Kgnn3bzg", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraStopByHandle);
};

} // namespace Libraries::Camera