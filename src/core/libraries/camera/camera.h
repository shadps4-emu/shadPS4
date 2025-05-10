// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Camera {

s32 PS4_SYSV_ABI module_start();
s32 PS4_SYSV_ABI module_stop();
s32 PS4_SYSV_ABI sceCameraAccGetData();
s32 PS4_SYSV_ABI sceCameraAudioClose();
s32 PS4_SYSV_ABI sceCameraAudioGetData();
s32 PS4_SYSV_ABI sceCameraAudioGetData2();
s32 PS4_SYSV_ABI sceCameraAudioOpen();
s32 PS4_SYSV_ABI sceCameraAudioReset();
s32 PS4_SYSV_ABI sceCameraChangeAppModuleState();
s32 PS4_SYSV_ABI sceCameraClose();
s32 PS4_SYSV_ABI sceCameraCloseByHandle();
s32 PS4_SYSV_ABI sceCameraDeviceOpen();
s32 PS4_SYSV_ABI sceCameraGetAttribute();
s32 PS4_SYSV_ABI sceCameraGetAutoExposureGain();
s32 PS4_SYSV_ABI sceCameraGetAutoWhiteBalance();
s32 PS4_SYSV_ABI sceCameraGetCalibData();
s32 PS4_SYSV_ABI sceCameraGetCalibDataFromDevice();
s32 PS4_SYSV_ABI sceCameraGetCalibrationData();
s32 PS4_SYSV_ABI sceCameraGetConfig();
s32 PS4_SYSV_ABI sceCameraGetContrast();
s32 PS4_SYSV_ABI sceCameraGetDefectivePixelCancellation();
s32 PS4_SYSV_ABI sceCameraGetDeviceConfig();
s32 PS4_SYSV_ABI sceCameraGetDeviceConfigWithoutHandle();
s32 PS4_SYSV_ABI sceCameraGetDeviceID();
s32 PS4_SYSV_ABI sceCameraGetDeviceIDWithoutOpen();
s32 PS4_SYSV_ABI sceCameraGetDeviceInfo();
s32 PS4_SYSV_ABI sceCameraGetExposureGain();
s32 PS4_SYSV_ABI sceCameraGetFrameData();
s32 PS4_SYSV_ABI sceCameraGetGamma();
s32 PS4_SYSV_ABI sceCameraGetHue();
s32 PS4_SYSV_ABI sceCameraGetLensCorrection();
s32 PS4_SYSV_ABI sceCameraGetMmapConnectedCount();
s32 PS4_SYSV_ABI sceCameraGetProductInfo();
s32 PS4_SYSV_ABI sceCameraGetRegister();
s32 PS4_SYSV_ABI sceCameraGetRegistryInfo();
s32 PS4_SYSV_ABI sceCameraGetSaturation();
s32 PS4_SYSV_ABI sceCameraGetSharpness();
s32 PS4_SYSV_ABI sceCameraGetVrCaptureInfo();
s32 PS4_SYSV_ABI sceCameraGetWhiteBalance();
s32 PS4_SYSV_ABI sceCameraInitializeRegistryCalibData();
s32 PS4_SYSV_ABI sceCameraIsAttached();
s32 PS4_SYSV_ABI sceCameraIsConfigChangeDone();
s32 PS4_SYSV_ABI sceCameraIsValidFrameData();
s32 PS4_SYSV_ABI sceCameraOpen();
s32 PS4_SYSV_ABI sceCameraOpenByModuleId();
s32 PS4_SYSV_ABI sceCameraRemoveAppModuleFocus();
s32 PS4_SYSV_ABI sceCameraSetAppModuleFocus();
s32 PS4_SYSV_ABI sceCameraSetAttribute();
s32 PS4_SYSV_ABI sceCameraSetAttributeInternal();
s32 PS4_SYSV_ABI sceCameraSetAutoExposureGain();
s32 PS4_SYSV_ABI sceCameraSetAutoWhiteBalance();
s32 PS4_SYSV_ABI sceCameraSetCalibData();
s32 PS4_SYSV_ABI sceCameraSetConfig();
s32 PS4_SYSV_ABI sceCameraSetConfigInternal();
s32 PS4_SYSV_ABI sceCameraSetContrast();
s32 PS4_SYSV_ABI sceCameraSetDebugStop();
s32 PS4_SYSV_ABI sceCameraSetDefectivePixelCancellation();
s32 PS4_SYSV_ABI sceCameraSetDefectivePixelCancellationInternal();
s32 PS4_SYSV_ABI sceCameraSetExposureGain();
s32 PS4_SYSV_ABI sceCameraSetForceActivate();
s32 PS4_SYSV_ABI sceCameraSetGamma();
s32 PS4_SYSV_ABI sceCameraSetHue();
s32 PS4_SYSV_ABI sceCameraSetLensCorrection();
s32 PS4_SYSV_ABI sceCameraSetLensCorrectionInternal();
s32 PS4_SYSV_ABI sceCameraSetProcessFocus();
s32 PS4_SYSV_ABI sceCameraSetProcessFocusByHandle();
s32 PS4_SYSV_ABI sceCameraSetRegister();
s32 PS4_SYSV_ABI sceCameraSetSaturation();
s32 PS4_SYSV_ABI sceCameraSetSharpness();
s32 PS4_SYSV_ABI sceCameraSetTrackerMode();
s32 PS4_SYSV_ABI sceCameraSetUacModeInternal();
s32 PS4_SYSV_ABI sceCameraSetVideoSync();
s32 PS4_SYSV_ABI sceCameraSetVideoSyncInternal();
s32 PS4_SYSV_ABI sceCameraSetWhiteBalance();
s32 PS4_SYSV_ABI sceCameraStart();
s32 PS4_SYSV_ABI sceCameraStartByHandle();
s32 PS4_SYSV_ABI sceCameraStop();
s32 PS4_SYSV_ABI sceCameraStopByHandle();

void RegisterlibSceCamera(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Camera