// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "library_common.h" 

namespace Libraries::Pad{

int PS4_SYSV_ABI scePadClose();
int PS4_SYSV_ABI scePadConnectPort();
int PS4_SYSV_ABI scePadDeviceClassGetExtendedInformation();
int PS4_SYSV_ABI scePadDeviceClassParseData();
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
int PS4_SYSV_ABI scePadGetControllerInformation();
int PS4_SYSV_ABI scePadGetDataInternal();
int PS4_SYSV_ABI scePadGetDeviceId();
int PS4_SYSV_ABI scePadGetDeviceInfo();
int PS4_SYSV_ABI scePadGetExtControllerInformation();
int PS4_SYSV_ABI scePadGetExtensionUnitInfo();
int PS4_SYSV_ABI scePadGetFeatureReport();
int PS4_SYSV_ABI scePadGetHandle();
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
int PS4_SYSV_ABI scePadOpen();
int PS4_SYSV_ABI scePadOpenExt();
int PS4_SYSV_ABI scePadOpenExt2();
int PS4_SYSV_ABI scePadOutputReport();
int PS4_SYSV_ABI scePadRead();
int PS4_SYSV_ABI scePadReadBlasterForTracker();
int PS4_SYSV_ABI scePadReadExt();
int PS4_SYSV_ABI scePadReadForTracker();
int PS4_SYSV_ABI scePadReadHistory();
int PS4_SYSV_ABI scePadReadState();
int PS4_SYSV_ABI scePadReadStateExt();
int PS4_SYSV_ABI scePadResetLightBar();
int PS4_SYSV_ABI scePadResetLightBarAll();
int PS4_SYSV_ABI scePadResetLightBarAllByPortType();
int PS4_SYSV_ABI scePadResetOrientation();
int PS4_SYSV_ABI scePadResetOrientationForTracker();
int PS4_SYSV_ABI scePadSetAngularVelocityDeadbandState();
int PS4_SYSV_ABI scePadSetAutoPowerOffCount();
int PS4_SYSV_ABI scePadSetButtonRemappingInfo();
int PS4_SYSV_ABI scePadSetConnection();
int PS4_SYSV_ABI scePadSetExtensionReport();
int PS4_SYSV_ABI scePadSetFeatureReport();
int PS4_SYSV_ABI scePadSetForceIntercepted();
int PS4_SYSV_ABI scePadSetLightBar();
int PS4_SYSV_ABI scePadSetLightBarBaseBrightness();
int PS4_SYSV_ABI scePadSetLightBarBlinking();
int PS4_SYSV_ABI scePadSetLightBarForTracker();
int PS4_SYSV_ABI scePadSetLoginUserNumber();
int PS4_SYSV_ABI scePadSetMotionSensorState();
int PS4_SYSV_ABI scePadSetProcessFocus();
int PS4_SYSV_ABI scePadSetProcessPrivilege();
int PS4_SYSV_ABI scePadSetProcessPrivilegeOfButtonRemapping();
int PS4_SYSV_ABI scePadSetShareButtonMaskForRemotePlay();
int PS4_SYSV_ABI scePadSetTiltCorrectionState();
int PS4_SYSV_ABI scePadSetUserColor();
int PS4_SYSV_ABI scePadSetVibration();
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

void RegisterlibScePad(Core::Loader::SymbolsResolver * sym);
}