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

enum OrbisSystemGestureTouchState : u32 {
    ORBIS_SYSTEM_GESTURE_TOUCH_STATE_INACTIVE = 0x00000000,
    ORBIS_SYSTEM_GESTURE_TOUCH_STATE_BEGIN = 0x00000001,
    ORBIS_SYSTEM_GESTURE_TOUCH_STATE_ACTIVE = 0x00000002,
    ORBIS_SYSTEM_GESTURE_TOUCH_STATE_END = 0x00000003,
    ORBIS_SYSTEM_GESTURE_TOUCH_STATE_CANCELLED = 0x00000004
};

enum OrbisSystemGestureType : u32 {
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
    // extra fields previous reserved
    u64 unknown_0x30; // time related
    u8 unknown_0x38[8];
    OrbisSystemGesturePrimitiveTouchEvent* next;
    u8 unknown_0x48[4];
    u8 condition_flag;
    u8 unknown_0x4D[3];
};

static_assert(sizeof(OrbisSystemGesturePrimitiveTouchEvent) == 0x50,
              "OrbisSystemGesturePrimitiveTouchEvent size must be 0x50");

struct OrbisSystemGesturePrimitiveTouchRecognizerParameter {
    u8 reserve[64];
};

struct OrbisSystemGestureTouchRecognizer {
    u32 field0_0x0;
    OrbisSystemGestureType gestureType;
    u64 field2_0x8;
    u64 updatedTime;
    u32 field4_0x18;
    u32 touchEventsCount;
    u64 field6_0x20;
    u64 field7_0x28;
    u64 field8_0x30;
    u64 field9_0x38;
    u64 field10_0x40;
    u64 field11_0x48;
    u64 field12_0x50;
    u64 field13_0x58;
    u64 field14_0x60;
    u64 field15_0x68;
    u64 field16_0x70;
    u64 field17_0x78;
    u64 field18_0x80;
    u64 field19_0x88;
    u64 field20_0x90;
    u64 field21_0x98;
    u64 field22_0xa0;
    u64 field23_0xa8;
    u64 field24_0xb0;
    u64 field25_0xb8;
    u64 field26_0xc0;
    u64 field27_0xc8;
    u64 field28_0xd0;
    u64 field29_0xd8;
    u64 field30_0xe0;
    u64 field31_0xe8;
    u64 field32_0xf0;
    u64 field33_0xf8;
    u64 field34_0x100;
    u64 field35_0x108;
    u64 field36_0x110;
    u64 field37_0x118;
    u64 field38_0x120;
    u64 field39_0x128;
    u64 field40_0x130;
    u64 field41_0x138;
    u64 field42_0x140;
    u64 field43_0x148;
    u64 field44_0x150;
    u64 field45_0x158;
    u64 field46_0x160;
    u64 field47_0x168;
    u64 field48_0x170;
    u64 field49_0x178;
    u64 field50_0x180;
    u64 field51_0x188;
    u64 field52_0x190;
    u64 field53_0x198;
    u64 field54_0x1a0;
    u64 field55_0x1a8;
    u64 field56_0x1b0;
    u64 field57_0x1b8;
    u64 field58_0x1c0;
    u64 field59_0x1c8;
    u64 field60_0x1d0;
    u64 field61_0x1d8;
    u64 field62_0x1e0;
    u64 field63_0x1e8;
    u64 field64_0x1f0;
    u64 field65_0x1f8;
    u64 field66_0x200;
    u64 field67_0x208;
    u64 field68_0x210;
    u64 field69_0x218;
    u64 field70_0x220;
    u64 field71_0x228;
    u64 field72_0x230;
    u64 field73_0x238;
    u64 field74_0x240;
    u64 field75_0x248;
    u64 field76_0x250;
    u64 field77_0x258;
    u64 field78_0x260;
    u64 field79_0x268;
    u64 field80_0x270;
    u64 field81_0x278;
    u64 field82_0x280;
    u64 field83_0x288;
    u64 field84_0x290;
    u64 field85_0x298;
    u64 field86_0x2a0;
    u64 field87_0x2a8;
    u64 field88_0x2b0;
    u64 field89_0x2b8;
    u64 field90_0x2c0;
    u64 field91_0x2c8;
    u64 field92_0x2d0;
    u64 field93_0x2d8;
    u64 field94_0x2e0;
    u64 field95_0x2e8;
    u64 field96_0x2f0;
    u64 field97_0x2f8;
    u64 field98_0x300;
    u64 field99_0x308;
    u64 field100_0x310;
    u64 field101_0x318;
    u64 field102_0x320;
    u64 field103_0x328;
    u64 field104_0x330;
    u64 field105_0x338;
    u64 field106_0x340;
    u64 field107_0x348;
    u64 field108_0x350;
    u64 field109_0x358;
    u64 field110_0x360;
    u64 field111_0x368;
    u64 field112_0x370;
    u64 field113_0x378;
    u64 field114_0x380;
    u64 field115_0x388;
    u64 field116_0x390;
    u64 field117_0x398;
    u64 field118_0x3a0;
    u64 field119_0x3a8;
    u64 field120_0x3b0;
    u64 field121_0x3b8;
    u64 field122_0x3c0;
    u64 field123_0x3c8;
    u64 field124_0x3d0;
    u64 field125_0x3d8;
    u64 field126_0x3e0;
    u64 field127_0x3e8;
    u64 field128_0x3f0;
    u64 field129_0x3f8;
    u64 field130_0x400;
    u64 field131_0x408;
    u64 field132_0x410;
    u64 field133_0x418;
    u64 field134_0x420;
    u64 field135_0x428;
    u64 field136_0x430;
    u64 field137_0x438;
    u64 field138_0x440;
    u64 field139_0x448;
    u64 field140_0x450;
    u64 field141_0x458;
    u64 field142_0x460;
    u64 field143_0x468;
    u64 field144_0x470;
    u64 field145_0x478;
    u64 field146_0x480;
    u64 field147_0x488;
    u64 field148_0x490;
    u64 field149_0x498;
    u64 field150_0x4a0;
    u64 field151_0x4a8;
    u64 field152_0x4b0;
    u64 field153_0x4b8;
    u64 field154_0x4c0;
    u64 field155_0x4c8;
    u64 field156_0x4d0;
    u64 field157_0x4d8;
    u64 field158_0x4e0;
    u64 field159_0x4e8;
    u64 field160_0x4f0;
    u64 field161_0x4f8;
    u64 field162_0x500;
    u64 field163_0x508;
    u64 field164_0x510;
    u64 field165_0x518;
    u64 field166_0x520;
    u64 field167_0x528;
    u64 field168_0x530;
    u64 field169_0x538;
    u64 field170_0x540;
    u64 field171_0x548;
    u64 field172_0x550;
    u64 field173_0x558;
    u64 field174_0x560;
    u64 field175_0x568;
    u64 field176_0x570;
    u64 field177_0x578;
    u64 field178_0x580;
    u64 field179_0x588;
    u64 field180_0x590;
    u64 field181_0x598;
    u64 field182_0x5a0;
    u64 field183_0x5a8;
    u64 field184_0x5b0;
    u64 field185_0x5b8;
    u64 field186_0x5c0;
    u64 field187_0x5c8;
    u64 field188_0x5d0;
    u64 field189_0x5d8;
    u64 field190_0x5e0;
    u64 field191_0x5e8;
    u64 field192_0x5f0;
    u64 field193_0x5f8;
    u64 field194_0x600;
    u64 field195_0x608;
    u64 field196_0x610;
    u64 field197_0x618;
    u64 field198_0x620;
    u64 field199_0x628;
    u64 field200_0x630;
    u64 field201_0x638;
    u64 field202_0x640;
    u64 field203_0x648;
    u64 field204_0x650;
    u64 field205_0x658;
    u64 field206_0x660;
    u64 field207_0x668;
    u64 field208_0x670;
    u64 field209_0x678;
    u64 field210_0x680;
    u64 field211_0x688;
    u64 field212_0x690;
    u64 field213_0x698;
    u64 field214_0x6a0;
    u64 field215_0x6a8;
    u64 field216_0x6b0;
    u64 field217_0x6b8;
    u64 field218_0x6c0;
    u64 field219_0x6c8;
    u64 field220_0x6d0;
    u64 field221_0x6d8;
    u64 field222_0x6e0;
    u64 field223_0x6e8;
    u64 field224_0x6f0;
    u64 field225_0x6f8;
    u64 field226_0x700;
    u64 field227_0x708;
    u64 field228_0x710;
    u64 field229_0x718;
    u64 field230_0x720;
    u64 field231_0x728;
    u64 field232_0x730;
    u64 field233_0x738;
    u64 field234_0x740;
    u64 field235_0x748;
    u64 field236_0x750;
    u64 field237_0x758;
    u64 field238_0x760;
    u64 field239_0x768;
    u64 field240_0x770;
    u64 field241_0x778;
    u64 field242_0x780;
    u64 field243_0x788;
    u64 field244_0x790;
    u64 field245_0x798;
    u64 field246_0x7a0;
    u64 field247_0x7a8;
    u64 field248_0x7b0;
    u64 field249_0x7b8;
    u64 field250_0x7c0;
    u64 field251_0x7c8;
    u64 field252_0x7d0;
    u64 field253_0x7d8;
    u64 field254_0x7e0;
    u64 field255_0x7e8;
    u64 field256_0x7f0;
    u64 field257_0x7f8;
    u64 field258_0x800;
    u64 field259_0x808;
    u64 field260_0x810;
    u64 field261_0x818;
    u64 field262_0x820;
    u64 field263_0x828;
    u64 field264_0x830;
    u64 field265_0x838;
    u64 field266_0x840;
    u64 field267_0x848;
    u64 field268_0x850;
    u64 field269_0x858;
    u64 field270_0x860;
    u64 field271_0x868;
    u64 field272_0x870;
    u64 field273_0x878;
    u64 field274_0x880;
    u64 field275_0x888;
    u64 field276_0x890;
    u64 field277_0x898;
    u64 field278_0x8a0;
    u64 field279_0x8a8;
    u64 field280_0x8b0;
    u64 field281_0x8b8;
    u64 field282_0x8c0;
    u64 field283_0x8c8;
    u64 field284_0x8d0;
    u64 field285_0x8d8;
    u64 field286_0x8e0;
    u64 field287_0x8e8;
    u64 field288_0x8f0;
    u64 field289_0x8f8;
    u64 field290_0x900;
    u64 field291_0x908;
    u64 field292_0x910;
    u64 field293_0x918;
    u64 field294_0x920;
    u64 field295_0x928;
    u64 field296_0x930;
    u64 field297_0x938;
    u64 field298_0x940;
    u64 field299_0x948;
    u64 field300_0x950;
    u64 field301_0x958;
    u64 field302_0x960;
    u64 field303_0x968;
    u64 field304_0x970;
    u64 field305_0x978;
    u64 field306_0x980;
    u64 field307_0x988;
    u64 field308_0x990;
    u64 field309_0x998;
    u64 field310_0x9a0;
    u64 field311_0x9a8;
    u64 field312_0x9b0;
    u64 field313_0x9b8;
    u64 field314_0x9c0;
    u64 field315_0x9c8;
    u64 field316_0x9d0;
    u64 field317_0x9d8;
    u64 field318_0x9e0;
    u64 field319_0x9e8;
    u64 field320_0x9f0;
    u64 field321_0x9f8;
    u64 field322_0xa00;
    u64 field323_0xa08;
    u64 field324_0xa10;
    u64 field325_0xa18;
    u64 field326_0xa20;
    u64 field327_0xa28;
    u64 field328_0xa30;
    u64 field329_0xa38;
    u64 field330_0xa40;
    u64 field331_0xa48;
    u64 field332_0xa50;
    u64 field333_0xa58;
    u64 field334_0xa60;
    u64 field335_0xa68;
    u64 field336_0xa70;
    u64 field337_0xa78;
    u64 field338_0xa80;
    u64 field339_0xa88;
    u64 field340_0xa90;
    u64 field341_0xa98;
    u8 dirtyflag; // 0xaa0
    u8 padd_0xaa1[3];
    float rectangle_x;
    float rectangle_y;
    float rectangle_width;
    float rectangle_height;
    u32 unknown1; // 0xab4
    u64 field349_0xab8;
    u64 field350_0xac0;
    u64 field351_0xac8;
    u64 field352_0xad0;
    u64 field353_0xad8;
    u64 field354_0xae0;
    u64 field355_0xae8;
    u64 field356_0xaf0;
    u64 field357_0xaf8;
    u32 unknown2; // 0xb00
    u32 magic;
    u64 field360_0xb08;
    u64 field361_0xb10;
    u64 field362_0xb18;
    u64 field363_0xb20;
    u64 field364_0xb28;
    u64 field365_0xb30;
    u64 field366_0xb38;
    u64 field367_0xb40;
};

static_assert(sizeof(OrbisSystemGestureTouchRecognizer) == 0xB48,
              "OrbisSystemGestureTouchRecognizer size must be 0xB48");

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

static_assert(sizeof(OrbisSystemGestureTouchEvent) == 0xA8,
              "OrbisSystemGestureTouchEvent size must be 0xA8");

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
