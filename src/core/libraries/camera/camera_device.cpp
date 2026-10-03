// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/camera/camera.h"

namespace Libraries::Camera {

s32 PS4_SYSV_ABI sceCameraDeviceOpen() {
    // Stubbed on real hardware
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
    config->sizeThis = sizeof(OrbisCameraConfig);
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

} // namespace Libraries::Camera
