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
              magic_enum::enum_integer(information->gestureType), information->updatedTime,
              information->rectangle.x, information->rectangle.y, information->rectangle.width,
              information->rectangle.height);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureGetTouchEventsCountInternal(
    s32 handle_index, OrbisSystemGestureTouchRecognizer* touchRecognizer) {

    LOG_TRACE(Lib_SystemGesture, "Called, handle_index={}, touchRecognizer={:p}", handle_index,
              fmt::ptr(touchRecognizer));

    // Check if handle_index is valid (0-7)
    if (handle_index >= 8) {
        LOG_ERROR(Lib_SystemGesture, "Invalid handle_index={}, must be < 8", handle_index);
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_HANDLE;
    }

    if (touchRecognizer == nullptr) {
        LOG_ERROR(Lib_SystemGesture, "touchRecognizer is null");
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_ARGUMENT;
    }

    // Check magic value in touchRecognizer
    if (touchRecognizer->magic != 0x35547435) { // ASCII "5Tt5"
        LOG_ERROR(Lib_SystemGesture,
                  "Invalid touchRecognizer magic, expected 0x35547435, got 0x{:x}",
                  touchRecognizer->magic);
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_ARGUMENT;
    }

    s32 touchEventsCount = touchRecognizer->touchEventsCount;

    LOG_TRACE(Lib_SystemGesture, "Touch events count: {}", touchEventsCount);

    return touchEventsCount;
}

s32 PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventsCountInternal(s32 handle_index) {

    LOG_TRACE(Lib_SystemGesture, "Called, handle_index={}", handle_index);

    // Check if handle_index is valid (0-7)
    if (handle_index >= 8) {
        LOG_ERROR(Lib_SystemGesture, "Invalid handle_index={}, must be < 8", handle_index);
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_HANDLE;
    }

    // Access primitive touch events count from global state
    u8 primitiveEventsCount = g_gestureGlobalState[handle_index].primitiveTouchEventsCount;

    LOG_TRACE(Lib_SystemGesture, "Primitive touch events count for handle {}: {}", handle_index,
              primitiveEventsCount);

    return static_cast<s32>(primitiveEventsCount);
}

s32 PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventByIndexInternal(
    s32 handle_index, u32 index, OrbisSystemGesturePrimitiveTouchEvent* primitiveTouchEvent) {

    LOG_TRACE(Lib_SystemGesture, "Called, handle_index={}, index={}", handle_index, index);

    // Check if handle_index is valid (0-7)
    if (handle_index >= 8) {
        LOG_ERROR(Lib_SystemGesture, "Invalid handle_index={}, must be < 8", handle_index);
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_HANDLE;
    }

    // Check if index is valid
    u32 total_events = g_gestureGlobalState[handle_index].primitiveTouchEventsCount;
    if (index >= total_events) {
        LOG_ERROR(Lib_SystemGesture, "Invalid index={}, total events={}", index, total_events);
        return ORBIS_SYSTEM_GESTURE_ERROR_INDEX_OUT_OF_ARRAY;
    }

    if (primitiveTouchEvent == nullptr) {
        LOG_ERROR(Lib_SystemGesture, "primitiveTouchEvent is null");
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_HANDLE;
    }

    // Get the 4 linked list heads
    OrbisSystemGestureGlobalStateInternal* global_state = &g_gestureGlobalState[handle_index];

    // Search through all 4 lists in order
    u32 current_index = 0;
    OrbisSystemGesturePrimitiveTouchEvent* found_event = nullptr;

    for (s32 list_idx = 0; list_idx < 4; list_idx++) {
        OrbisSystemGesturePrimitiveTouchEvent* current =
            global_state->primitiveEventLists[list_idx];

        while (current != nullptr) {
            if (current_index == index) {
                found_event = current;
                break;
            }
            current_index++;
            current = current->next;
        }

        if (found_event != nullptr) {
            break;
        }
    }

    if (found_event == nullptr) {
        LOG_ERROR(Lib_SystemGesture, "Event at index={} not found", index);
        return ORBIS_SYSTEM_GESTURE_ERROR_INDEX_OUT_OF_ARRAY;
    }

    // Clear output (safer, though decompilation doesn't show it for this function)
    std::memset(primitiveTouchEvent, 0, sizeof(OrbisSystemGesturePrimitiveTouchEvent));

    // Copy fields as shown in decompilation
    primitiveTouchEvent->primitiveID = found_event->primitiveID;
    primitiveTouchEvent->eventState = found_event->eventState;
    primitiveTouchEvent->pressedPosition = found_event->pressedPosition;
    primitiveTouchEvent->currentPosition = found_event->currentPosition;

    // Conditional deltaVector copy
    if (found_event->condition_flag == 0) {
        primitiveTouchEvent->deltaVector = found_event->deltaVector;
    } else {
        primitiveTouchEvent->deltaVector.x = 0.0f;
        primitiveTouchEvent->deltaVector.y = 0.0f;
    }

    primitiveTouchEvent->isUpdated = found_event->isUpdated;

    // Field swap: deltaTime gets elapsedTime, elapsedTime gets unknown_0x30
    primitiveTouchEvent->deltaTime = found_event->elapsedTime;
    primitiveTouchEvent->elapsedTime = found_event->unknown_0x30;

    LOG_TRACE(Lib_SystemGesture,
              "Found primitive event at index={}: ID={}, state={}, "
              "pressed=({},{}), current=({},{}), deltaTime={}, elapsedTime={}",
              index, primitiveTouchEvent->primitiveID,
              static_cast<u32>(primitiveTouchEvent->eventState),
              primitiveTouchEvent->pressedPosition.x, primitiveTouchEvent->pressedPosition.y,
              primitiveTouchEvent->currentPosition.x, primitiveTouchEvent->currentPosition.y,
              primitiveTouchEvent->deltaTime, primitiveTouchEvent->elapsedTime);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventByPrimitiveIDInternal(
    s32 handle_index, u16 primitiveID, OrbisSystemGesturePrimitiveTouchEvent* primitiveTouchEvent) {

    LOG_TRACE(Lib_SystemGesture, "Called, handle_index={}, primitiveID={}", handle_index,
              primitiveID);

    // Check if handle_index is valid (0-7)
    if (handle_index >= 8) {
        LOG_ERROR(Lib_SystemGesture, "Invalid handle_index={}, must be < 8", handle_index);
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_HANDLE; // 0x80890001
    }

    // Get the 4 linked list heads
    OrbisSystemGestureGlobalStateInternal* global_state = &g_gestureGlobalState[handle_index];
    OrbisSystemGesturePrimitiveTouchEvent* list0 = global_state->primitiveEventLists[0];
    OrbisSystemGesturePrimitiveTouchEvent* list1 = global_state->primitiveEventLists[1];
    OrbisSystemGesturePrimitiveTouchEvent* list2 = global_state->primitiveEventLists[2];
    OrbisSystemGesturePrimitiveTouchEvent* list3 = global_state->primitiveEventLists[3];

    LOG_TRACE(Lib_SystemGesture, "List pointers: list0={}, list1={}, list2={}, list3={}",
              static_cast<void*>(list0), static_cast<void*>(list1), static_cast<void*>(list2),
              static_cast<void*>(list3));

    OrbisSystemGesturePrimitiveTouchEvent* found_event = nullptr;
    s32 list_found_in = -1;

    // Search through list 0
    OrbisSystemGesturePrimitiveTouchEvent* current = list0;
    s32 list_index = 0;
    while (current != nullptr) {
        if (current->primitiveID == primitiveID) {
            found_event = current;
            list_found_in = 0;
            break;
        }
        current = current->next;
        list_index++;
    }

    // Search through list 1
    if (found_event == nullptr) {
        list_index = 0;
        current = list1;
        while (current != nullptr) {
            if (current->primitiveID == primitiveID) {
                found_event = current;
                list_found_in = 1;
                break;
            }
            current = current->next;
            list_index++;
        }
    }

    // Search through list 2
    if (found_event == nullptr) {
        list_index = 0;
        current = list2;
        while (current != nullptr) {
            if (current->primitiveID == primitiveID) {
                found_event = current;
                list_found_in = 2;
                break;
            }
            current = current->next;
            list_index++;
        }
    }

    // Search through list 3
    if (found_event == nullptr) {
        list_index = 0;
        current = list3;
        while (current != nullptr) {
            if (current->primitiveID == primitiveID) {
                found_event = current;
                list_found_in = 3;
                break;
            }
            current = current->next;
            list_index++;
        }
    }

    // Check if event was found
    if (found_event == nullptr) {
        LOG_ERROR(Lib_SystemGesture, "Primitive ID {} not found in any list", primitiveID);
        return ORBIS_SYSTEM_GESTURE_ERROR_EVENT_DATA_NOT_FOUND;
    }

    LOG_TRACE(Lib_SystemGesture, "Found primitive ID {} in list {}, position {} in list",
              primitiveID, list_found_in, list_index);

    // Check if output buffer is provided
    if (primitiveTouchEvent == nullptr) {
        LOG_ERROR(Lib_SystemGesture, "primitiveTouchEvent is null");
        return ORBIS_SYSTEM_GESTURE_ERROR_INVALID_HANDLE;
    }

    // Copy fields from found event to output buffer
    primitiveTouchEvent->primitiveID =
        primitiveID; // Note: uses parameter, not found_event->primitiveID
    primitiveTouchEvent->eventState = found_event->eventState;
    primitiveTouchEvent->pressedPosition = found_event->pressedPosition;
    primitiveTouchEvent->currentPosition = found_event->currentPosition;

    // Conditional deltaVector copy
    if (found_event->condition_flag == 0) {
        primitiveTouchEvent->deltaVector = found_event->deltaVector;
        LOG_TRACE(Lib_SystemGesture, "condition_flag=0, copying deltaVector: x={}, y={}",
                  primitiveTouchEvent->deltaVector.x, primitiveTouchEvent->deltaVector.y);
    } else {
        primitiveTouchEvent->deltaVector.x = 0.0f;
        primitiveTouchEvent->deltaVector.y = 0.0f;
        LOG_TRACE(Lib_SystemGesture, "condition_flag={}, zeroing deltaVector",
                  found_event->condition_flag);
    }

    primitiveTouchEvent->isUpdated = found_event->isUpdated;

    primitiveTouchEvent->deltaTime = found_event->elapsedTime;
    primitiveTouchEvent->elapsedTime = found_event->unknown_0x30;

    LOG_TRACE(Lib_SystemGesture,
              "Copied event: ID={}, state={}, "
              "pressed=({},{}), current=({},{}), deltaTime={}, elapsedTime={}, isUpdated={}",
              primitiveTouchEvent->primitiveID, static_cast<u32>(primitiveTouchEvent->eventState),
              primitiveTouchEvent->pressedPosition.x, primitiveTouchEvent->pressedPosition.y,
              primitiveTouchEvent->currentPosition.x, primitiveTouchEvent->currentPosition.y,
              primitiveTouchEvent->deltaTime, primitiveTouchEvent->elapsedTime,
              primitiveTouchEvent->isUpdated);

    LOG_TRACE(Lib_SystemGesture, "Successfully returned primitive touch event for ID={}",
              primitiveID);
    return ORBIS_OK;
}

} // namespace Libraries::SystemGesture