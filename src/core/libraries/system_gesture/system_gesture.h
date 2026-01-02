// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <core/libraries/pad/pad.h>
#include "common/enum.h"
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SystemGesture {

constexpr int ORBIS_SYSTEM_GESTURE_INPUT_TOUCH_PAD = 0;

enum OrbisSystemGestureTouchState {
    ORBIS_SYSTEM_GESTURE_TOUCH_STATE_INACTIVE = 0x00000000,
    ORBIS_SYSTEM_GESTURE_TOUCH_STATE_BEGIN = 0x00000001,
    ORBIS_SYSTEM_GESTURE_TOUCH_STATE_ACTIVE = 0x00000002,
    ORBIS_SYSTEM_GESTURE_TOUCH_STATE_END = 0x00000003,
    ORBIS_SYSTEM_GESTURE_TOUCH_STATE_CANCELLED = 0x00000004
};

enum OrbisSystemGestureType {
    ORBIS_SYSTEM_GESTURE_TYPE_TAP = 0x00000001,
    ORBIS_SYSTEM_GESTURE_TYPE_DRAG = 0x00000002,
    ORBIS_SYSTEM_GESTURE_TYPE_TAP_AND_HOLD = 0x00000004,
    ORBIS_SYSTEM_GESTURE_TYPE_PINCH_OUT_IN = 0x00000008,
    ORBIS_SYSTEM_GESTURE_TYPE_ROTATION = 0x00000010,
    ORBIS_SYSTEM_GESTURE_TYPE_FLICK = 0x00000020
};

struct OrbisSystemGestureVector2 {
    float x;
    float y;
};

struct OrbisSystemGestureRectangle {
    float x;
    float y;
    float width;
    float height;
    u8 reserve[8];
};

struct OrbisSystemGesturePrimitiveTouchEvent {
    OrbisSystemGestureTouchState eventState;
    u16 primitiveID;

    u8 isUpdated;
    u8 reserved0;

    OrbisSystemGestureVector2 pressedPosition;
    OrbisSystemGestureVector2 currentPosition;
    OrbisSystemGestureVector2 deltaVector;

    u64 deltaTime;
    u64 elapsedTime;

    u8 reserve[32];
};

struct OrbisSystemGesturePrimitiveTouchRecognizerParameter {
    u8 reserve[64];
};

struct OrbisSystemGestureTouchRecognizer {
    u64 reserve[361];
};

struct OrbisSystemGestureTouchRecognizerInformation {
    OrbisSystemGestureType gestureType;
    OrbisSystemGestureRectangle rectangle;
    u64 updatedTime;

    u8 reserve[256];
};

struct OrbisSystemGestureOpenParameter {
    u8 reserve[8];
};

struct OrbisSystemGestureTapEventProperty {
    u16 primitiveID;
    OrbisSystemGestureVector2 position;
    u8 tappedCount;

    u8 reserve[72];
};

struct OrbisSystemGestureDragEventProperty {
    u16 primitiveID;
    OrbisSystemGestureVector2 deltaVector;
    OrbisSystemGestureVector2 currentPosition;
    OrbisSystemGestureVector2 pressedPosition;

    u8 reserve[60];
};

struct OrbisSystemGestureTapAndHoldEventProperty {
    u16 primitiveID;
    OrbisSystemGestureVector2 pressedPosition;

    u8 reserve[76];
};

struct OrbisSystemGesturePinchOutInEventProperty {
    float scale;

    struct {
        u16 primitiveID;
        OrbisSystemGestureVector2 currentPosition;
        OrbisSystemGestureVector2 deltaVector;
        OrbisSystemGestureVector2 pairedPosition;
    } primitive[2];

    u8 reserve[28];
};

struct OrbisSystemGestureRotationEventProperty {
    float angle;

    struct {
        u16 primitiveID;
        OrbisSystemGestureVector2 currentPosition;
        OrbisSystemGestureVector2 deltaVector;
        OrbisSystemGestureVector2 pairedPosition;
    } primitive[2];

    u8 reserve[28];
};

struct OrbisSystemGestureFlickEventProperty {
    u16 primitiveID;
    OrbisSystemGestureVector2 deltaVector;
    OrbisSystemGestureVector2 releasedPosition;
    OrbisSystemGestureVector2 pressedPosition;

    u8 reserve[60];
};

struct OrbisSystemGestureTouchEvent {
    u32 eventID;
    OrbisSystemGestureTouchState eventState;
    OrbisSystemGestureType gestureType;

    u8 isUpdated;
    u8 padding[3];
    u64 updatedTime;

    union {
        u8 propertyBuf[88];

        OrbisSystemGestureTapEventProperty tap;
        OrbisSystemGestureDragEventProperty drag;
        OrbisSystemGestureTapAndHoldEventProperty tapAndHold;
        OrbisSystemGesturePinchOutInEventProperty pinchOutIn;
        OrbisSystemGestureRotationEventProperty rotation;
        OrbisSystemGestureFlickEventProperty flick;
    } property;

    u8 reserve[56];
};

struct OrbisSystemGestureTapRecognizerParameter {
    u8 maxTapCount;
    u8 reserve[63];
};

struct OrbisSystemGestureDragRecognizerParameter {
    u8 reserve[64];
};

struct OrbisSystemGestureTapAndHoldRecognizerParameter {
    u64 timeToInvokeEvent;
    u8 reserve[56];
};

struct OrbisSystemGesturePinchOutInRecognizerParameter {
    u8 reserve[64];
};

struct OrbisSystemGestureRotationRecognizerParameter {
    u8 reserve[64];
};

struct OrbisSystemGestureFlickRecognizerParameter {
    u8 reserve[64];
};

union OrbisSystemGestureTouchRecognizerParameter {
    u8 parameterBuf[64];

    OrbisSystemGestureTapRecognizerParameter tap;
    OrbisSystemGestureDragRecognizerParameter drag;
    OrbisSystemGestureTapAndHoldRecognizerParameter tapAndHold;
    OrbisSystemGesturePinchOutInRecognizerParameter pinchOutIn;
    OrbisSystemGestureRotationRecognizerParameter rotation;
    OrbisSystemGestureFlickRecognizerParameter flick;
};

struct OrbisSystemGestureTouchPadData {
    s32 padHandle;
    s32 reportNumber;
    Libraries::Pad::OrbisPadData* padDataBuffer;
    u8 reserve[8];
};

s32 PS4_SYSV_ABI sceSystemGestureAppendTouchRecognizer(
    s32 gestureHandle, OrbisSystemGestureTouchRecognizer* touchRecognizer);
s32 PS4_SYSV_ABI sceSystemGestureClose(s32 gestureHandle);
s32 PS4_SYSV_ABI sceSystemGestureCreateTouchRecognizer(
    s32 gestureHandle, OrbisSystemGestureTouchRecognizer* touchRecognizer,
    OrbisSystemGestureType gestureType, OrbisSystemGestureRectangle* rectangle,
    OrbisSystemGestureTouchRecognizerParameter* touchRecognizerParameter);
s32 PS4_SYSV_ABI sceSystemGestureFinalizePrimitiveTouchRecognizer();
s32 PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventByIndex(
    s32 gestureHandle, const u32 index, OrbisSystemGesturePrimitiveTouchEvent* primitiveTouchEvent);
s32 PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(
    s32 gestureHandle, const u16 primitiveID,
    OrbisSystemGesturePrimitiveTouchEvent* primitiveTouchEvent);
s32 PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEvents(
    s32 gestureHandle, OrbisSystemGesturePrimitiveTouchEvent* primitiveEventBuffer,
    const u32 capacityOfBuffer, u32* numberOfEvent);
s32 PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventsCount(s32 gestureHandle);
s32 PS4_SYSV_ABI sceSystemGestureGetTouchEventByEventID(
    s32 gestureHandle, const OrbisSystemGestureTouchRecognizer* touchRecognizer, const u32 eventID,
    OrbisSystemGestureTouchEvent* touchEvent);
s32 PS4_SYSV_ABI sceSystemGestureGetTouchEventByIndex(
    s32 gestureHandle, const OrbisSystemGestureTouchRecognizer* touchRecognizer, const u32 eventID,
    OrbisSystemGestureTouchEvent* touchEvent);
s32 PS4_SYSV_ABI sceSystemGestureGetTouchEvents(
    s32 gestureHandle, const OrbisSystemGestureTouchRecognizer* touchRecognizer,
    OrbisSystemGestureTouchEvent* eventBuffer, const u32 capacityOfBuffer, u32* numberOfEvent);
s32 PS4_SYSV_ABI sceSystemGestureGetTouchEventsCount(
    s32 gestureHandle, const OrbisSystemGestureTouchRecognizer* touchRecognizer);
s32 PS4_SYSV_ABI sceSystemGestureGetTouchRecognizerInformation(
    s32 gestureHandle, const OrbisSystemGestureTouchRecognizer* touchRecognizer,
    OrbisSystemGestureTouchRecognizerInformation* information);
s32 PS4_SYSV_ABI sceSystemGestureInitializePrimitiveTouchRecognizer(
    OrbisSystemGesturePrimitiveTouchRecognizerParameter* parameter);
s32 PS4_SYSV_ABI sceSystemGestureOpen(s32 inputType, OrbisSystemGestureOpenParameter* parameter);
s32 PS4_SYSV_ABI sceSystemGestureRemoveTouchRecognizer(
    s32 gestureHandle, OrbisSystemGestureTouchRecognizer* touchRecognizer);
s32 PS4_SYSV_ABI sceSystemGestureResetPrimitiveTouchRecognizer(s32 gestureHandle);
s32 PS4_SYSV_ABI sceSystemGestureResetTouchRecognizer(
    s32 gestureHandle, OrbisSystemGestureTouchRecognizer* touchRecognizer);
s32 PS4_SYSV_ABI sceSystemGestureUpdateAllTouchRecognizer(s32 gestureHandle);
s32 PS4_SYSV_ABI sceSystemGestureUpdatePrimitiveTouchRecognizer(
    s32 gestureHandle, const OrbisSystemGestureTouchPadData* pInputData);
s32 PS4_SYSV_ABI sceSystemGestureUpdateTouchRecognizer(
    s32 gestureHandle, OrbisSystemGestureTouchRecognizer* touchRecognizer);
s32 PS4_SYSV_ABI sceSystemGestureUpdateTouchRecognizerRectangle(
    s32 gestureHandle, OrbisSystemGestureTouchRecognizer* touchRecognizer,
    const OrbisSystemGestureRectangle* rectangle);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SystemGesture
