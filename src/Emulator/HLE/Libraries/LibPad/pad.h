#pragma once
#include <Emulator/HLE/Libraries/LibUserService/user_service.h>
#include <types.h>

#include "Core/PS4/Loader/SymbolsResolver.h"

namespace Emulator::HLE::Libraries::LibPad {

typedef enum : u32 {
    SCE_PAD_BUTTON_L3 = 0x00000002,
    SCE_PAD_BUTTON_R3 = 0x00000004,
    SCE_PAD_BUTTON_OPTIONS = 0x00000008,
    SCE_PAD_BUTTON_UP = 0x00000010,
    SCE_PAD_BUTTON_RIGHT = 0x00000020,
    SCE_PAD_BUTTON_DOWN = 0x00000040,
    SCE_PAD_BUTTON_LEFT = 0x00000080,
    SCE_PAD_BUTTON_L2 = 0x00000100,
    SCE_PAD_BUTTON_R2 = 0x00000200,
    SCE_PAD_BUTTON_L1 = 0x00000400,
    SCE_PAD_BUTTON_R1 = 0x00000800,
    SCE_PAD_BUTTON_TRIANGLE = 0x00001000,
    SCE_PAD_BUTTON_CIRCLE = 0x00002000,
    SCE_PAD_BUTTON_CROSS = 0x00004000,
    SCE_PAD_BUTTON_SQUARE = 0x00008000,
    SCE_PAD_BUTTON_TOUCH_PAD = 0x00100000,
    SCE_PAD_BUTTON_INTERCEPTED = 0x80000000,
} ScePadButton;

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
// hle functions
int PS4_SYSV_ABI scePadInit();
int PS4_SYSV_ABI scePadOpen(Emulator::HLE::Libraries::LibUserService::SceUserServiceUserId userId, s32 type, s32 index,
                            const ScePadOpenParam* pParam);
int PS4_SYSV_ABI scePadReadState(int32_t handle, ScePadData* pData);

void libPad_Register(SymbolsResolver* sym);
};  // namespace Emulator::HLE::Libraries::LibPad