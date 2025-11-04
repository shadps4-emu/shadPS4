// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/libs.h"
#include "core/libraries/pad/pad.h"
#include "core/libraries/pad/pad_errors.h"
#include "input/controller.h"
#include "system_gesture.h"

namespace Libraries::SystemGesture {

using Input::GameController;

int PS4_SYSV_ABI sceSystemGestureAppendTouchRecognizer() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureClose() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureCreateTouchRecognizer() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureFinalizePrimitiveTouchRecognizer() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventByIndex() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventByPrimitiveID() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEvents() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventsCount() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureGetTouchEventByEventID() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureGetTouchEventByIndex() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureGetTouchEvents() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureGetTouchEventsCount() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureGetTouchRecognizerInformation() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureInitializePrimitiveTouchRecognizer() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureOpen() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureRemoveTouchRecognizer() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureResetPrimitiveTouchRecognizer() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureResetTouchRecognizer() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureUpdateAllTouchRecognizer() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureUpdatePrimitiveTouchRecognizer() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureUpdateTouchRecognizer() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemGestureUpdateTouchRecognizerRectangle() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("1MMK0W-kMgA", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureAppendTouchRecognizer);
    LIB_FUNCTION("j4yXIA2jJ68", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureClose);
    LIB_FUNCTION("FWF8zkhr854", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureCreateTouchRecognizer);
    LIB_FUNCTION("3QYCmMlOlCY", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureFinalizePrimitiveTouchRecognizer);
    LIB_FUNCTION("KAeP0+cQPVU", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureGetPrimitiveTouchEventByIndex);
    LIB_FUNCTION("yBaQ0h9m1NM", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureGetPrimitiveTouchEventByPrimitiveID);
    LIB_FUNCTION("L8YmemOeSNY", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureGetPrimitiveTouchEvents);
    LIB_FUNCTION("JhwByySf9FY", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureGetPrimitiveTouchEventsCount);
    LIB_FUNCTION("lpsXm7tzeoc", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureGetTouchEventByEventID);
    LIB_FUNCTION("TSKvgSz5ChU", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureGetTouchEventByIndex);
    LIB_FUNCTION("fLTseA7XiWY", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureGetTouchEvents);
    LIB_FUNCTION("h8uongcBNVs", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureGetTouchEventsCount);
    LIB_FUNCTION("0KrW5eMnrwY", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureGetTouchRecognizerInformation);
    LIB_FUNCTION("3pcAvmwKCvM", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureInitializePrimitiveTouchRecognizer);
    LIB_FUNCTION("qpo-mEOwje0", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureOpen);
    LIB_FUNCTION("ELvBVG-LKT0", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureRemoveTouchRecognizer);
    LIB_FUNCTION("o11J529VaAE", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureResetPrimitiveTouchRecognizer);
    LIB_FUNCTION("oBuH3zFWYIg", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureResetTouchRecognizer);
    LIB_FUNCTION("wPJGwI2RM2I", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureUpdateAllTouchRecognizer);
    LIB_FUNCTION("GgFMb22sbbI", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureUpdatePrimitiveTouchRecognizer);
    LIB_FUNCTION("j4h82CQWENo", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureUpdateTouchRecognizer);
    LIB_FUNCTION("4WOA1eTx3V8", "libSceSystemGesture", 1, "libSceSystemGesture",
                 sceSystemGestureUpdateTouchRecognizerRectangle);
}

} // namespace Libraries::SystemGesture
