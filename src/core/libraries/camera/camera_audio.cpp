// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/camera/camera.h"

namespace Libraries::Camera {

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

} // namespace Libraries::Camera