#include "system_gesture_internal.h"
// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/logging/log.h>
#include <core/libraries/error_codes.h>
#include <magic_enum/magic_enum.hpp>
#include "system_gesture.h"
#include "system_gesture_error.h"
#include "system_gesture_internal.h"

namespace Libraries::SystemGesture {

s32 PS4_SYSV_ABI sceSystemGestureUpdateTouchRecognizerRectangleInternal(
    s32 handle_index, OrbisSystemGestureTouchRecognizer* touchRecognizer,
    OrbisSystemGestureRectangle* rectangle) {

    LOG_TRACE(Lib_SystemGesture, "Called, handle_index={}, touchRecognizer={:p}, rectangle={:p}",
              handle_index, fmt::ptr(touchRecognizer), fmt::ptr(rectangle));

    // Check if handle_index is valid (0-7)
    if (handle_index >= 8) {
        LOG_ERROR(Lib_SystemGesture, "Invalid handle_index={}, must be < 8", handle_index);
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_HANDLE;
    }

    if (touchRecognizer == nullptr) {
        LOG_ERROR(Lib_SystemGesture, "touchRecognizer is null");
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_ARGUMENT;
    }

    if (rectangle == nullptr) {
        LOG_ERROR(Lib_SystemGesture, "rectangle is null");
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_ARGUMENT;
    }

    // Check magic value in touchRecognizer
    if (touchRecognizer->magic != 0x35547435) { // ASCII "5Tt5"
        LOG_ERROR(Lib_SystemGesture,
                  "Invalid touchRecognizer magic, expected 0x35547435, got 0x{:x}",
                  touchRecognizer->magic);
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_ARGUMENT;
    }

    // Copy rectangle fields individually
    touchRecognizer->rectangle_x = rectangle->x;
    touchRecognizer->rectangle_y = rectangle->y;
    touchRecognizer->rectangle_width = rectangle->width;
    touchRecognizer->rectangle_height = rectangle->height;

    // Clear the dirty flag
    touchRecognizer->dirtyflag = 0;

    LOG_TRACE(Lib_SystemGesture,
              "Updated touch recognizer rectangle: x={}, y={}, width={}, height={}", rectangle->x,
              rectangle->y, rectangle->width, rectangle->height);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureGetTouchRecognizerInformationInternal(
    s32 handle_index, OrbisSystemGestureTouchRecognizer* touchRecognizer,
    OrbisSystemGestureTouchRecognizerInformation* information) {

    LOG_TRACE(Lib_SystemGesture, "Called, handle_index={}, touchRecognizer={:p}, information={:p}",
              handle_index, fmt::ptr(touchRecognizer), fmt::ptr(information));

    // Check if handle_index is valid (0-7)
    if (handle_index >= 8) {
        LOG_ERROR(Lib_SystemGesture, "Invalid handle_index={}, must be < 8", handle_index);
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_HANDLE;
    }

    if (touchRecognizer == nullptr) {
        LOG_ERROR(Lib_SystemGesture, "touchRecognizer is null");
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_ARGUMENT;
    }

    if (information == nullptr) {
        LOG_ERROR(Lib_SystemGesture, "information is null");
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_ARGUMENT;
    }

    // Check magic value in touchRecognizer
    if (touchRecognizer->magic != 0x35547435) { // ASCII "5Tt5"
        LOG_ERROR(Lib_SystemGesture,
                  "Invalid touchRecognizer magic, expected 0x35547435, got 0x{:x}",
                  touchRecognizer->magic);
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_ARGUMENT;
    }
    // Copy gesture type
    information->gestureType = touchRecognizer->gestureType;

    // Copy update time
    information->updatedTime = touchRecognizer->updatedTime;

    // Copy rectangle information
    information->rectangle.x = touchRecognizer->rectangle_x;
    information->rectangle.y = touchRecognizer->rectangle_y;
    information->rectangle.width = touchRecognizer->rectangle_width;
    information->rectangle.height = touchRecognizer->rectangle_height;

    LOG_TRACE(Lib_SystemGesture,
              "Retrieved touch recognizer information: gestureType=(0x{:x}), "
              "updateTime={}, rectangle x={}, y={}, width={}, height={}",
               magic_enum::enum_integer(information->gestureType), information->updatedTime, information->rectangle.x,
              information->rectangle.y, information->rectangle.width,
              information->rectangle.height);

    return ORBIS_OK;
}

} // namespace Libraries::SystemGesture