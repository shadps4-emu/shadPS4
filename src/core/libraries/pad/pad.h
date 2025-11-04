// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Pad {

constexpr int ORBIS_PAD_MAX_TOUCH_NUM = 2;
constexpr int ORBIS_PAD_MAX_DEVICE_UNIQUE_DATA_SIZE = 12;

constexpr int ORBIS_PAD_PORT_TYPE_STANDARD = 0;
constexpr int ORBIS_PAD_PORT_TYPE_SPECIAL = 2;
constexpr int ORBIS_PAD_PORT_TYPE_REMOTE_CONTROL = 16;

enum class OrbisPadDeviceClass {
    Invalid = -1,
    Standard = 0,
    Guitar = 1,
    Drum = 2,
    DjTurntable = 3,
    Dancemat = 4,
    Navigation = 5,
    SteeringWheel = 6,
    Stick = 7,
    FightStick = 8,
    Gun = 9,
};

struct OrbisPadDeviceClassExtendedInformation {
    OrbisPadDeviceClass deviceClass;
    u8 reserved[4];
    union {
        struct {
            u8 capability;
            u8 reserved1[1];
            u16 maxPhysicalWheelAngle;
            u8 reserved2[8];
        } steeringWheel;
        struct {
            u8 capability;
            u8 quantityOfSelectorSwitch;
            u8 reserved[10];
        } guitar;
        struct {
            u8 capability;
            u8 reserved[11];
        } drum;
        struct {
            u8 capability;
            u8 reserved[11];
        } flightStick;
        u8 data[12];
    } classData;
};

struct OrbisPadDeviceClassData {
    OrbisPadDeviceClass deviceClass;
    bool bDataValid;
    union {
        struct {
            float steeringWheelAngle;
            u16 steeringWheel;
            u16 acceleratorPedal;
            u16 brakePedal;
            u16 clutchPedal;
            u16 handBrake;
            u8 gear;
            u8 reserved[1];
        } steeringWheel;
        struct {
            u8 toneNumber;
            u8 whammyBar;
            u8 tilt;
            u8 fret;
            u8 fretSolo;
            u8 reserved[11];
        } guitar;
        struct {
            u8 snare;
            u8 tom1;
            u8 tom2;
            u8 floorTom;
            u8 hihatCymbal;
            u8 rideCymbal;
            u8 crashCymbal;
            u8 reserved[9];
        } drum;
        struct {
            u16 stickAxisX;
            u16 stickAxisY;
            u8 stickTwist;
            u8 throttle;
            u8 trigger;
            u8 rudderPedal;
            u8 brakePedalLeft;
            u8 brakePedalRight;
            u8 antennaKnob;
            u8 rangeKnob;
            u8 reserved[4];
        } flightStick;
        struct {
            u8 dataLen;
            u8 reserved[3];
            u8 data[ORBIS_PAD_MAX_DEVICE_UNIQUE_DATA_SIZE];
        } others;
    } classData;
};

struct OrbisPadAnalogButtons {
    u8 l2;
    u8 r2;
    u8 padding[2];
};

struct OrbisPadAnalogStick {
    u8 x;
    u8 y;
};

enum class OrbisPadButtonDataOffset : u32 {
    None = 0,
    L3 = 0x2,
    R3 = 0x4,
    Options = 0x8,
    Up = 0x10,
    Right = 0x20,
    Down = 0x40,
    Left = 0x80,
    L2 = 0x100,
    R2 = 0x200,
    L1 = 0x400,
    R1 = 0x800,
    Triangle = 0x1000,
    Circle = 0x2000,
    Cross = 0x4000,
    Square = 0x8000,
    TouchPad = 0x100000,
    Intercepted = 0x80000000,
};
DECLARE_ENUM_FLAG_OPERATORS(OrbisPadButtonDataOffset)

struct OrbisFQuaternion {
    float x, y, z, w;
};

struct OrbisFVector3 {
    float x, y, z;
};

struct OrbisPadTouch {
    u16 x;
    u16 y;
    u8 id;
    u8 reserve[3];
};

struct OrbisPadTouchData {
    u8 touchNum;
    u8 reserve[3];
    u32 reserve1;
    OrbisPadTouch touch[ORBIS_PAD_MAX_TOUCH_NUM];
};

struct OrbisPadExtensionUnitData {
    u32 extensionUnitId;
    u8 reserve[1];
    u8 dataLength;
    u8 data[10];
};

struct OrbisPadData {
    OrbisPadButtonDataOffset buttons;
    OrbisPadAnalogStick leftStick;
    OrbisPadAnalogStick rightStick;
    OrbisPadAnalogButtons analogButtons;
    OrbisFQuaternion orientation;
    OrbisFVector3 acceleration;
    OrbisFVector3 angularVelocity;
    OrbisPadTouchData touchData;
    bool connected;
    u64 timestamp;
    OrbisPadExtensionUnitData extensionUnitData;
    u8 connectedCount;
    u8 reserve[2];
    u8 deviceUniqueDataLen;
    u8 deviceUniqueData[ORBIS_PAD_MAX_DEVICE_UNIQUE_DATA_SIZE];
};

struct OrbisPadTouchPadInformation {
    float pixelDensity;
    struct {
        u16 x;
        u16 y;
    } resolution;
};

struct OrbisPadStickInformation {
    u8 deadZoneLeft;
    u8 deadZoneRight;
};

struct OrbisPadControllerInformation {
    OrbisPadTouchPadInformation touchPadInfo;
    OrbisPadStickInformation stickInfo;
    u8 connectionType;
    u8 connectedCount;
    bool connected;
    OrbisPadDeviceClass deviceClass;
    u8 reserve[8];
};

struct OrbisPadExtendedControllerInformation {
    OrbisPadControllerInformation base;
    u16 padType1;
    u16 padType2;
    u8 capability;

    union {
        u8 quantityOfSelectorSwitch;
        int maxPhysicalWheelAngle;
        u8 data[8];
    };
};

struct OrbisPadOpenParam {
    u8 reserve[8];
};

struct OrbisPadOpenExtParam {
    u16 vendorId;
    u16 productId;
    u16 productId_2;
    u8 reserve[10];
};

struct OrbisPadLightBarParam {
    u8 r;
    u8 g;
    u8 b;
    u8 reserve[1];
};

struct OrbisPadVibrationParam {
    u8 largeMotor;
    u8 smallMotor;
};

int PS4_SYSV_ABI scePadClose(s32 handle);
int PS4_SYSV_ABI scePadConnectPort();
int PS4_SYSV_ABI scePadDeviceClassGetExtendedInformation(
    s32 handle, OrbisPadDeviceClassExtendedInformation* pExtInfo);
int PS4_SYSV_ABI scePadDeviceClassParseData(s32 handle, const OrbisPadData* pData,
                                            OrbisPadDeviceClassData* pDeviceClassData);
int PS4_SYSV_ABI scePadDeviceOpen();
int PS4_SYSV_ABI scePadDisableVibration();
int PS4_SYSV_ABI scePadDisconnectDevice();
int PS4_SYSV_ABI scePadDisconnectPort();
int PS4_SYSV_ABI scePadEnableAutoDetect();
int PS4_SYSV_ABI scePadEnableExtensionPort();
int PS4_SYSV_ABI scePadEnableSpecificDeviceClass();
int PS4_SYSV_ABI scePadEnableUsbConnection();
int PS4_SYSV_ABI scePadGetBluetoothAddress();
int PS4_SYSV_ABI scePadGetCapability();
int PS4_SYSV_ABI scePadGetControllerInformation(s32 handle, OrbisPadControllerInformation* pInfo);
int PS4_SYSV_ABI scePadGetDataInternal();
int PS4_SYSV_ABI scePadGetDeviceId();
int PS4_SYSV_ABI scePadGetDeviceInfo();
int PS4_SYSV_ABI scePadGetExtControllerInformation(s32 handle,
                                                   OrbisPadExtendedControllerInformation* pInfo);
int PS4_SYSV_ABI scePadGetExtensionUnitInfo();
int PS4_SYSV_ABI scePadGetFeatureReport();
int PS4_SYSV_ABI scePadGetHandle(s32 userId, s32 type, s32 index);
int PS4_SYSV_ABI scePadGetIdleCount();
int PS4_SYSV_ABI scePadGetInfo();
int PS4_SYSV_ABI scePadGetInfoByPortType();
int PS4_SYSV_ABI scePadGetLicenseControllerInformation();
int PS4_SYSV_ABI scePadGetMotionSensorPosition();
int PS4_SYSV_ABI scePadGetMotionTimerUnit();
int PS4_SYSV_ABI scePadGetSphereRadius();
int PS4_SYSV_ABI scePadGetVersionInfo();
int PS4_SYSV_ABI scePadInit();
int PS4_SYSV_ABI scePadIsBlasterConnected();
int PS4_SYSV_ABI scePadIsDS4Connected();
int PS4_SYSV_ABI scePadIsLightBarBaseBrightnessControllable();
int PS4_SYSV_ABI scePadIsMoveConnected();
int PS4_SYSV_ABI scePadIsMoveReproductionModel();
int PS4_SYSV_ABI scePadIsValidHandle();
int PS4_SYSV_ABI scePadMbusInit();
int PS4_SYSV_ABI scePadMbusTerm();
int PS4_SYSV_ABI scePadOpen(s32 userId, s32 type, s32 index, const OrbisPadOpenParam* pParam);
int PS4_SYSV_ABI scePadOpenExt(s32 userId, s32 type, s32 index, const OrbisPadOpenExtParam* pParam);
int PS4_SYSV_ABI scePadOpenExt2();
int PS4_SYSV_ABI scePadOutputReport();
int PS4_SYSV_ABI scePadRead(s32 handle, OrbisPadData* pData, s32 num);
int PS4_SYSV_ABI scePadReadBlasterForTracker();
int PS4_SYSV_ABI scePadReadExt();
int PS4_SYSV_ABI scePadReadForTracker();
int PS4_SYSV_ABI scePadReadHistory();
int PS4_SYSV_ABI scePadReadState(s32 handle, OrbisPadData* pData);
int PS4_SYSV_ABI scePadReadStateExt();
int PS4_SYSV_ABI scePadResetLightBar(s32 handle);
int PS4_SYSV_ABI scePadResetLightBarAll();
int PS4_SYSV_ABI scePadResetLightBarAllByPortType();
int PS4_SYSV_ABI scePadResetOrientation(s32 handle);
int PS4_SYSV_ABI scePadResetOrientationForTracker();
int PS4_SYSV_ABI scePadSetAngularVelocityDeadbandState(s32 handle, bool bEnable);
int PS4_SYSV_ABI scePadSetAutoPowerOffCount();
int PS4_SYSV_ABI scePadSetButtonRemappingInfo();
int PS4_SYSV_ABI scePadSetConnection();
int PS4_SYSV_ABI scePadSetExtensionReport();
int PS4_SYSV_ABI scePadSetFeatureReport();
int PS4_SYSV_ABI scePadSetForceIntercepted();
int PS4_SYSV_ABI scePadSetLightBar(s32 handle, const OrbisPadLightBarParam* pParam);
int PS4_SYSV_ABI scePadSetLightBarBaseBrightness();
int PS4_SYSV_ABI scePadSetLightBarBlinking();
int PS4_SYSV_ABI scePadSetLightBarForTracker();
int PS4_SYSV_ABI scePadSetLoginUserNumber();
int PS4_SYSV_ABI scePadSetMotionSensorState(s32 handle, bool bEnable);
int PS4_SYSV_ABI scePadSetProcessFocus();
int PS4_SYSV_ABI scePadSetProcessPrivilege();
int PS4_SYSV_ABI scePadSetProcessPrivilegeOfButtonRemapping();
int PS4_SYSV_ABI scePadSetShareButtonMaskForRemotePlay();
int PS4_SYSV_ABI scePadSetTiltCorrectionState(s32 handle, bool bEnable);
int PS4_SYSV_ABI scePadSetUserColor();
int PS4_SYSV_ABI scePadSetVibration(s32 handle, const OrbisPadVibrationParam* pParam);
int PS4_SYSV_ABI scePadSetVibrationForce();
int PS4_SYSV_ABI scePadSetVrTrackingMode();
int PS4_SYSV_ABI scePadShareOutputData();
int PS4_SYSV_ABI scePadStartRecording();
int PS4_SYSV_ABI scePadStopRecording();
int PS4_SYSV_ABI scePadSwitchConnection();
int PS4_SYSV_ABI scePadVertualDeviceAddDevice();
int PS4_SYSV_ABI scePadVirtualDeviceAddDevice();
int PS4_SYSV_ABI scePadVirtualDeviceDeleteDevice();
int PS4_SYSV_ABI scePadVirtualDeviceDisableButtonRemapping();
int PS4_SYSV_ABI scePadVirtualDeviceGetRemoteSetting();
int PS4_SYSV_ABI scePadVirtualDeviceInsertData();
int PS4_SYSV_ABI Func_28B998C7D8A3DA1D();
int PS4_SYSV_ABI Func_298D21481F94C9FA();
int PS4_SYSV_ABI Func_51E514BCD3A05CA5();
int PS4_SYSV_ABI Func_89C9237E393DA243();
int PS4_SYSV_ABI Func_EF103E845B6F0420();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Pad
