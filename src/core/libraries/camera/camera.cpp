// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/camera/camera.h"
#include "core/libraries/camera/camera_error.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::Camera {

s32 PS4_SYSV_ABI sceCameraAccGetData() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraAudioClose() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraAudioGetData() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraAudioGetData2() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraAudioOpen() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraAudioReset() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraChangeAppModuleState() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraClose(s32 handle) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraCloseByHandle() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraDeviceOpen() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetAttribute(s32 handle, OrbisCameraAttribute* pAttribute) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetAutoExposureGain(s32 handle, OrbisCameraChannel channel, u32* pEnable,
                                              void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetAutoWhiteBalance(s32 handle, OrbisCameraChannel channel, u32* pEnable,
                                              void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetCalibData() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetCalibDataFromDevice() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetCalibrationData() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetConfig(s32 handle, OrbisCameraConfig* pConfig) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetContrast(s32 handle, OrbisCameraChannel channel, u32* pContrast,
                                      void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetDefectivePixelCancellation(s32 handle, OrbisCameraChannel channel,
                                                        u32* pEnable, void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetDeviceConfig() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetDeviceConfigWithoutHandle() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetDeviceID() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetDeviceIDWithoutOpen() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetDeviceInfo(s32 reserved, OrbisCameraDeviceInfo* pDeviceInfo) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetExposureGain(s32 handle, OrbisCameraChannel channel,
                                          OrbisCameraExposureGain* pExposureGain, void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetFrameData(int handle, OrbisCameraFrameData* pFrameData) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetGamma(s32 handle, OrbisCameraChannel channel, OrbisCameraGamma* pGamma,
                                   void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetHue(s32 handle, OrbisCameraChannel channel, s32* pHue, void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetLensCorrection(s32 handle, OrbisCameraChannel channel, u32* pEnable,
                                            void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetMmapConnectedCount() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetProductInfo() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetRegister() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetRegistryInfo() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetSaturation(s32 handle, OrbisCameraChannel channel, u32* pSaturation,
                                        void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetSharpness(s32 handle, OrbisCameraChannel channel, u32* pSharpness,
                                       void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetVrCaptureInfo() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraGetWhiteBalance(s32 handle, OrbisCameraChannel channel,
                                          OrbisCameraWhiteBalance* pWhiteBalance, void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraInitializeRegistryCalibData() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraIsAttached(s32 index) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraIsConfigChangeDone() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraIsValidFrameData(int handle, OrbisCameraFrameData* pFrameData) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraOpen(Libraries::UserService::OrbisUserServiceUserId userId, s32 type,
                               s32 index, OrbisCameraOpenParameter* pParam) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraOpenByModuleId() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraRemoveAppModuleFocus() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetAppModuleFocus() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetAttribute(s32 handle, OrbisCameraAttribute* pAttribute) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetAttributeInternal() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetAutoExposureGain(s32 handle, OrbisCameraChannel channel, u32 enable,
                                              void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetAutoWhiteBalance(s32 handle, OrbisCameraChannel channel, u32 enable,
                                              void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetCalibData() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetConfig(s32 handle, OrbisCameraConfig* pConfig) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetConfigInternal() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetContrast(s32 handle, OrbisCameraChannel channel, u32 contrast,
                                      void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetDebugStop() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetDefectivePixelCancellation(s32 handle, OrbisCameraChannel channel,
                                                        u32 enable, void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetDefectivePixelCancellationInternal() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetExposureGain(s32 handle, OrbisCameraChannel channel,
                                          OrbisCameraExposureGain* pExposureGain, void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetForceActivate() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetGamma(s32 handle, OrbisCameraChannel channel, OrbisCameraGamma* pGamma,
                                   void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetHue(s32 handle, OrbisCameraChannel channel, s32 hue, void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetLensCorrection(s32 handle, OrbisCameraChannel channel, u32 enable,
                                            void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetLensCorrectionInternal() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetProcessFocus() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetProcessFocusByHandle() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetRegister() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetSaturation(s32 handle, OrbisCameraChannel channel, u32 saturation,
                                        void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetSharpness(s32 handle, OrbisCameraChannel channel, u32 sharpness,
                                       void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetTrackerMode() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetUacModeInternal() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetVideoSync(s32 handle, OrbisCameraVideoSyncParameter* pVideoSync) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetVideoSyncInternal() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraSetWhiteBalance(s32 handle, OrbisCameraChannel channel,
                                          OrbisCameraWhiteBalance* pWhiteBalance, void* pOption) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraStart(s32 handle, OrbisCameraStartParameter* pParam) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraStartByHandle() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraStop(s32 handle) {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCameraStopByHandle() {
    LOG_ERROR(Lib_Camera, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("QhjrPkRPUZQ", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraAccGetData);
    LIB_FUNCTION("UFonL7xopFM", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraAudioClose);
    LIB_FUNCTION("fkZE7Hup2ro", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraAudioGetData);
    LIB_FUNCTION("hftC5A1C8OQ", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraAudioGetData2);
    LIB_FUNCTION("DhqqFiBU+6g", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraAudioOpen);
    LIB_FUNCTION("wyU98EXAYxU", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraAudioReset);
    LIB_FUNCTION("Y0pCDajzkVQ", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraChangeAppModuleState);
    LIB_FUNCTION("OMS9LlcrvBo", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraClose);
    LIB_FUNCTION("ztqH5qNTpTk", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraCloseByHandle);
    LIB_FUNCTION("nBH6i2s4Glc", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraDeviceOpen);
    LIB_FUNCTION("0btIPD5hg5A", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetAttribute);
    LIB_FUNCTION("oEi6vM-3E2c", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetAutoExposureGain);
    LIB_FUNCTION("qTPRMh4eY60", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetAutoWhiteBalance);
    LIB_FUNCTION("hHA1frlMxYE", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetCalibData);
    LIB_FUNCTION("5Oie5RArfWs", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetCalibDataFromDevice);
    LIB_FUNCTION("RHYJ7GKOSMg", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetCalibrationData);
    LIB_FUNCTION("ZaqmGEtYuL0", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetConfig);
    LIB_FUNCTION("a5xFueMZIMs", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetContrast);
    LIB_FUNCTION("tslCukqFE+E", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetDefectivePixelCancellation);
    LIB_FUNCTION("DSOLCrc3Kh8", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetDeviceConfig);
    LIB_FUNCTION("n+rFeP1XXyM", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetDeviceConfigWithoutHandle);
    LIB_FUNCTION("jTJCdyv9GLU", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetDeviceID);
    LIB_FUNCTION("-H3UwGQvNZI", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetDeviceIDWithoutOpen);
    LIB_FUNCTION("WZpxnSAM-ds", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetDeviceInfo);
    LIB_FUNCTION("ObIste7hqdk", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetExposureGain);
    LIB_FUNCTION("mxgMmR+1Kr0", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetFrameData);
    LIB_FUNCTION("WVox2rwGuSc", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetGamma);
    LIB_FUNCTION("zrIUDKZx0iE", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetHue);
    LIB_FUNCTION("XqYRHc4aw3w", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetLensCorrection);
    LIB_FUNCTION("B260o9pSzM8", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraGetMmapConnectedCount);
    LIB_FUNCTION("ULxbwqiYYuU", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetProductInfo);
    LIB_FUNCTION("olojYZKYiYs", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetRegister);
    LIB_FUNCTION("hawKak+Auw4", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetRegistryInfo);
    LIB_FUNCTION("RTDOsWWqdME", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetSaturation);
    LIB_FUNCTION("c6Fp9M1EXXc", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetSharpness);
    LIB_FUNCTION("IAz2HgZQWzE", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetVrCaptureInfo);
    LIB_FUNCTION("HX5524E5tMY", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraGetWhiteBalance);
    LIB_FUNCTION("0wnf2a60FqI", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraInitializeRegistryCalibData);
    LIB_FUNCTION("p6n3Npi3YY4", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraIsAttached);
    LIB_FUNCTION("wQfd7kfRZvo", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraIsConfigChangeDone);
    LIB_FUNCTION("U3BVwQl2R5Q", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraIsValidFrameData);
    LIB_FUNCTION("BHn83xrF92E", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraOpen);
    LIB_FUNCTION("eTywOSWsEiI", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraOpenByModuleId);
    LIB_FUNCTION("py8p6kZcHmA", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraRemoveAppModuleFocus);
    LIB_FUNCTION("j5isFVIlZLk", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetAppModuleFocus);
    LIB_FUNCTION("doPlf33ab-U", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetAttribute);
    LIB_FUNCTION("96F7zp1Xo+k", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetAttributeInternal);
    LIB_FUNCTION("yfSdswDaElo", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetAutoExposureGain);
    LIB_FUNCTION("zIKL4kZleuc", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetAutoWhiteBalance);
    LIB_FUNCTION("LEMk5cTHKEA", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetCalibData);
    LIB_FUNCTION("VQ+5kAqsE2Q", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetConfig);
    LIB_FUNCTION("9+SNhbctk64", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetConfigInternal);
    LIB_FUNCTION("3i5MEzrC1pg", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetContrast);
    LIB_FUNCTION("vejouEusC7g", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetDebugStop);
    LIB_FUNCTION("jMv40y2A23g", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetDefectivePixelCancellation);
    LIB_FUNCTION("vER3cIMBHqI", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetDefectivePixelCancellationInternal);
    LIB_FUNCTION("wgBMXJJA6K4", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetExposureGain);
    LIB_FUNCTION("jeTpU0MqKU0", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetForceActivate);
    LIB_FUNCTION("lhEIsHzB8r4", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetGamma);
    LIB_FUNCTION("QI8GVJUy2ZY", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetHue);
    LIB_FUNCTION("K7W7H4ZRwbc", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetLensCorrection);
    LIB_FUNCTION("eHa3vhGu2rQ", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetLensCorrectionInternal);
    LIB_FUNCTION("lS0tM6n+Q5E", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetProcessFocus);
    LIB_FUNCTION("NVITuK83Z7o", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetProcessFocusByHandle);
    LIB_FUNCTION("8MjO05qk5hA", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetRegister);
    LIB_FUNCTION("bSKEi2PzzXI", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetSaturation);
    LIB_FUNCTION("P-7MVfzvpsM", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetSharpness);
    LIB_FUNCTION("3VJOpzKoIeM", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetTrackerMode);
    LIB_FUNCTION("nnR7KAIDPv8", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetUacModeInternal);
    LIB_FUNCTION("wpeyFwJ+UEI", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetVideoSync);
    LIB_FUNCTION("8WtmqmE4edw", "libSceCamera", 1, "libSceCamera", 1, 1,
                 sceCameraSetVideoSyncInternal);
    LIB_FUNCTION("k3zPIcgFNv0", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraSetWhiteBalance);
    LIB_FUNCTION("9EpRYMy7rHU", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraStart);
    LIB_FUNCTION("cLxF1QtHch0", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraStartByHandle);
    LIB_FUNCTION("2G2C0nmd++M", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraStop);
    LIB_FUNCTION("+X1Kgnn3bzg", "libSceCamera", 1, "libSceCamera", 1, 1, sceCameraStopByHandle);
};

} // namespace Libraries::Camera