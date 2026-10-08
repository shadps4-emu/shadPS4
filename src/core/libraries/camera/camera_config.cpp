// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/camera/camera.h"

namespace Libraries::Camera {

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
    if (option != nullptr && (g_firmware_version < Common::ElfInfo::FW_300 ||
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
    exposure_gain->exposure = 20;
    exposure_gain->gain = 100;
    exposure_gain->mode = 0;
    return ORBIS_OK;
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
        if (g_firmware_version < Common::ElfInfo::FW_300 ||
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
    LOG_INFO(Lib_Camera, "called");
    if (handle < 1 || config == nullptr || config->sizeThis != sizeof(OrbisCameraConfig)) {
        LOG_ERROR(Lib_Camera, "ORBIS_CAMERA_ERROR_PARAM");
        return ORBIS_CAMERA_ERROR_PARAM;
    }
    if (!g_library_opened) {
        LOG_ERROR(Lib_Camera, "ORBIS_CAMERA_ERROR_NOT_OPEN");
        return ORBIS_CAMERA_ERROR_NOT_OPEN;
    }
    if (EmulatorSettings.GetCameraId() == -1) {
        LOG_ERROR(Lib_Camera, "ORBIS_CAMERA_ERROR_NOT_CONNECTED");
        return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
    }

    switch (config->configType) {
    case ORBIS_CAMERA_CONFIG_TYPE1:
    case ORBIS_CAMERA_CONFIG_TYPE2:
    case ORBIS_CAMERA_CONFIG_TYPE3:
    case ORBIS_CAMERA_CONFIG_TYPE4:
        output_config0 = camera_config_types[config->configType - 1][0];
        output_config1 = camera_config_types[config->configType - 1][1];
        break;
    case ORBIS_CAMERA_CONFIG_TYPE5:
        if (g_firmware_version < Common::ElfInfo::FW_450) {
            LOG_ERROR(Lib_Camera, "ORBIS_CAMERA_ERROR_UNKNOWN_CONFIG");
            return ORBIS_CAMERA_ERROR_UNKNOWN_CONFIG;
        }
        output_config0 = camera_config_types[config->configType - 1][0];
        output_config1 = camera_config_types[config->configType - 1][1];
        break;
    case ORBIS_CAMERA_CONFIG_EXTENTION:
        output_config0 = config->configExtention[0];
        output_config1 = config->configExtention[1];
        break;
    default:
        LOG_ERROR(Lib_Camera, "Invalid config type {}", std::to_underlying(config->configType));
        return ORBIS_CAMERA_ERROR_PARAM;
    }

    return ORBIS_OK;
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
    if (EmulatorSettings.GetCameraId() == -1) {
        return ORBIS_CAMERA_ERROR_NOT_CONNECTED;
    }

    return ORBIS_OK;
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
    if (g_firmware_version >= Common::ElfInfo::FW_350 && sharpness > 10) {
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

} // namespace Libraries::Camera