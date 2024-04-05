// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "src/core/libraries/libsceuserservice.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace OldLibraries::LibPad {

#define ORBIS_PAD_PORT_TYPE_STANDARD 0
#define ORBIS_PAD_PORT_TYPE_SPECIAL 2

#define ORBIS_PAD_DEVICE_CLASS_PAD 0
#define ORBIS_PAD_DEVICE_CLASS_GUITAR 1
#define ORBIS_PAD_DEVICE_CLASS_DRUMS 2

#define ORBIS_PAD_CONNECTION_TYPE_STANDARD 0
#define ORBIS_PAD_CONNECTION_TYPE_REMOTE 2

enum ScePadButton : u32 {
    L3 = 0x00000002,
    R3 = 0x00000004,
    OPTIONS = 0x00000008,
    UP = 0x00000010,
    RIGHT = 0x00000020,
    DOWN = 0x00000040,
    LEFT = 0x00000080,
    L2 = 0x00000100,
    R2 = 0x00000200,
    L1 = 0x00000400,
    R1 = 0x00000800,
    TRIANGLE = 0x00001000,
    CIRCLE = 0x00002000,
    CROSS = 0x00004000,
    SQUARE = 0x00008000,
    TOUCH_PAD = 0x00100000,
    INTERCEPTED = 0x80000000,
};

struct ScePadOpenParam {
    u8 reserve[8];
};

struct ScePadAnalogStick {
    u8 x;
    u8 y;
};
struct ScePadAnalogButtons {
    u8 l2;
    u8 r2;
    u8 padding[2];
};

struct SceFQuaternion {
    float x, y, z, w;
};

struct SceFVector3 {
    float x, y, z;
};

struct ScePadTouch {
    u16 x;
    u16 y;
    u8 id;
    u8 reserve[3];
};

constexpr int SCE_PAD_MAX_TOUCH_NUM = 2;

typedef struct ScePadTouchData {
    u8 touchNum;
    u8 reserve[3];
    u32 reserve1;
    ScePadTouch touch[SCE_PAD_MAX_TOUCH_NUM];
} ScePadTouchData;

struct ScePadExtensionUnitData {
    u32 extensionUnitId;
    u8 reserve[1];
    u8 dataLength;
    u8 data[10];
};

struct ScePadData {
    u32 buttons;
    ScePadAnalogStick leftStick;
    ScePadAnalogStick rightStick;
    ScePadAnalogButtons analogButtons;
    SceFQuaternion orientation;
    SceFVector3 acceleration;
    SceFVector3 angularVelocity;
    ScePadTouchData touchData;
    bool connected;
    u64 timestamp;
    ScePadExtensionUnitData extensionUnitData;
    uint8_t connectedCount;
    uint8_t reserve[2];
    uint8_t deviceUniqueDataLen;
    uint8_t deviceUniqueData[12];
};

struct OrbisPadInformation {
    float touchpadDensity;
    u16 touchResolutionX;
    u16 touchResolutionY;
    u8 stickDeadzoneL;
    u8 stickDeadzoneR;
    u8 connectionType;
    u8 count;
    s8 connected;
    s8 deviceClass;
    u8 unknown[8];
};

int PS4_SYSV_ABI scePadInit();
int PS4_SYSV_ABI scePadOpen(Libraries::UserService::OrbisUserServiceUserId userId, s32 type,
                            s32 index, const ScePadOpenParam* pParam);
int PS4_SYSV_ABI scePadReadState(int32_t handle, ScePadData* pData);

void padSymbolsRegister(Core::Loader::SymbolsResolver* sym);

}; // namespace OldLibraries::LibPad
