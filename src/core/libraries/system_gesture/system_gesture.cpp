// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <core/libraries/error_codes.h>
#include "common/config.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/libs.h"
#include "system_gesture.h"
#include "system_gesture_error.h"

namespace Libraries::SystemGesture {

s32 PS4_SYSV_ABI sceSystemGestureAppendTouchRecognizer(
    s32 gestureHandle, OrbisSystemGestureTouchRecognizer* touchRecognizer) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureClose(s32 gestureHandle) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureCreateTouchRecognizer(
    s32 gestureHandle, OrbisSystemGestureTouchRecognizer* touchRecognizer,
    OrbisSystemGestureType gestureType, OrbisSystemGestureRectangle* rectangle,
    OrbisSystemGestureTouchRecognizerParameter* touchRecognizerParameter) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureFinalizePrimitiveTouchRecognizer() {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventByIndex(
    s32 gestureHandle, const u32 index,
    OrbisSystemGesturePrimitiveTouchEvent* primitiveTouchEvent) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(
    s32 gestureHandle, const uint16_t primitiveID,
    OrbisSystemGesturePrimitiveTouchEvent* primitiveTouchEvent) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEvents(
    s32 gestureHandle, OrbisSystemGesturePrimitiveTouchEvent* primitiveEventBuffer,
    const u32 capacityOfBuffer, u32* numberOfEvent) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventsCount(s32 gestureHandle) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureGetTouchEventByEventID(
    s32 gestureHandle, const OrbisSystemGestureTouchRecognizer* touchRecognizer, const u32 eventID,
    OrbisSystemGestureTouchEvent* touchEvent) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureGetTouchEventByIndex(
    s32 gestureHandle, const OrbisSystemGestureTouchRecognizer* touchRecognizer, const u32 eventID,
    OrbisSystemGestureTouchEvent* touchEvent) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureGetTouchEvents(
    s32 gestureHandle, const OrbisSystemGestureTouchRecognizer* touchRecognizer,
    OrbisSystemGestureTouchEvent* eventBuffer, const u32 capacityOfBuffer, u32* numberOfEvent) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureGetTouchEventsCount(
    s32 gestureHandle, const OrbisSystemGestureTouchRecognizer* touchRecognizer) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureGetTouchRecognizerInformation(
    s32 gestureHandle, const OrbisSystemGestureTouchRecognizer* touchRecognizer,
    OrbisSystemGestureTouchRecognizerInformation* information) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureInitializePrimitiveTouchRecognizer(
    OrbisSystemGesturePrimitiveTouchRecognizerParameter* parameter) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureOpen(s32 inputType, OrbisSystemGestureOpenParameter* parameter) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureRemoveTouchRecognizer(
    s32 gestureHandle, OrbisSystemGestureTouchRecognizer* touchRecognizer) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureResetPrimitiveTouchRecognizer(s32 gestureHandle) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureResetTouchRecognizer(
    s32 gestureHandle, OrbisSystemGestureTouchRecognizer* touchRecognizer) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureUpdateAllTouchRecognizer(s32 gestureHandle) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureUpdatePrimitiveTouchRecognizer(
    s32 gestureHandle, const OrbisSystemGestureTouchPadData* pInputData) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureUpdateTouchRecognizer(
    s32 gestureHandle, OrbisSystemGestureTouchRecognizer* touchRecognizer) {
    LOG_ERROR(Lib_SystemGesture, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureUpdateTouchRecognizerRectangle(
    s32 gestureHandle, OrbisSystemGestureTouchRecognizer* touchRecognizer,
    const OrbisSystemGestureRectangle* rectangle) {
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
