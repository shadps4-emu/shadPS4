#pragma once

#include "common/types.h"
#include "core/hle/libraries/libuserservice/libuserservice.h"

namespace Core::Libraries::LibPad {

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
    u08 reserve[8];
};

struct ScePadAnalogStick {
    u08 x;
    u08 y;
};
struct ScePadAnalogButtons {
    u08 l2;
    u08 r2;
    u08 padding[2];
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
    u08 id;
    u08 reserve[3];
};

constexpr int SCE_PAD_MAX_TOUCH_NUM = 2;

typedef struct ScePadTouchData {
    u08 touchNum;
    u08 reserve[3];
    u32 reserve1;
    ScePadTouch touch[SCE_PAD_MAX_TOUCH_NUM];
} ScePadTouchData;

struct ScePadExtensionUnitData {
    u32 extensionUnitId;
    u08 reserve[1];
    u08 dataLength;
    u08 data[10];
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

int PS4_SYSV_ABI scePadInit();
int PS4_SYSV_ABI scePadOpen(LibUserService::SceUserServiceUserId userId, s32 type, s32 index,
                            const ScePadOpenParam* pParam);
int PS4_SYSV_ABI scePadReadState(int32_t handle, ScePadData* pData);

void padSymbolsRegister(Loader::SymbolsResolver* sym);

}; // namespace Core::Libraries::LibPad
