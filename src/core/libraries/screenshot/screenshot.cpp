// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "screenshot.h"

namespace Libraries::ScreenShot {

int PS4_SYSV_ABI _Z5dummyv() {
    LOG_ERROR(Lib_Screenshot, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceScreenShotCapture() {
    LOG_ERROR(Lib_Screenshot, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceScreenShotDisable() {
    LOG_ERROR(Lib_Screenshot, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceScreenShotDisableNotification() {
    LOG_ERROR(Lib_Screenshot, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceScreenShotEnable() {
    LOG_ERROR(Lib_Screenshot, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceScreenShotEnableNotification() {
    LOG_ERROR(Lib_Screenshot, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceScreenShotGetAppInfo() {
    LOG_ERROR(Lib_Screenshot, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceScreenShotGetDrcParam() {
    LOG_ERROR(Lib_Screenshot, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceScreenShotIsDisabled() {
    LOG_ERROR(Lib_Screenshot, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceScreenShotIsVshScreenCaptureDisabled() {
    LOG_ERROR(Lib_Screenshot, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceScreenShotSetOverlayImage() {
    LOG_ERROR(Lib_Screenshot, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceScreenShotSetOverlayImageWithOrigin() {
    LOG_ERROR(Lib_Screenshot, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceScreenShotSetParam() {
    LOG_ERROR(Lib_Screenshot, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceScreenShotSetDrcParam() {
    LOG_ERROR(Lib_Screenshot, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("AS45QoYHjc4", "libSceScreenShot", 1, "libSceScreenShot", _Z5dummyv);
    LIB_FUNCTION("JuMLLmmvRgk", "libSceScreenShot", 1, "libSceScreenShot", sceScreenShotCapture);
    LIB_FUNCTION("tIYf0W5VTi8", "libSceScreenShot", 1, "libSceScreenShot", sceScreenShotDisable);
    LIB_FUNCTION("ysfza71rm9M", "libSceScreenShot", 1, "libSceScreenShot",
                 sceScreenShotDisableNotification);
    LIB_FUNCTION("2xxUtuC-RzE", "libSceScreenShot", 1, "libSceScreenShot", sceScreenShotEnable);
    LIB_FUNCTION("BDUaqlVdSAY", "libSceScreenShot", 1, "libSceScreenShot",
                 sceScreenShotEnableNotification);
    LIB_FUNCTION("hNmK4SdhPT0", "libSceScreenShot", 1, "libSceScreenShot", sceScreenShotGetAppInfo);
    LIB_FUNCTION("VlAQIgXa2R0", "libSceScreenShot", 1, "libSceScreenShot",
                 sceScreenShotGetDrcParam);
    LIB_FUNCTION("-SV-oTNGFQk", "libSceScreenShot", 1, "libSceScreenShot", sceScreenShotIsDisabled);
    LIB_FUNCTION("ICNJ-1POs84", "libSceScreenShot", 1, "libSceScreenShot",
                 sceScreenShotIsVshScreenCaptureDisabled);
    LIB_FUNCTION("ahHhOf+QNkQ", "libSceScreenShot", 1, "libSceScreenShot",
                 sceScreenShotSetOverlayImage);
    LIB_FUNCTION("73WQ4Jj0nJI", "libSceScreenShot", 1, "libSceScreenShot",
                 sceScreenShotSetOverlayImageWithOrigin);
    LIB_FUNCTION("G7KlmIYFIZc", "libSceScreenShot", 1, "libSceScreenShot", sceScreenShotSetParam);
    LIB_FUNCTION("itlWFWV3Tzc", "libSceScreenShotDrc", 1, "libSceScreenShot",
                 sceScreenShotSetDrcParam);
};

} // namespace Libraries::ScreenShot