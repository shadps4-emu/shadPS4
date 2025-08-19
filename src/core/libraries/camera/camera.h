// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <core/libraries/system/userservice.h>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Camera {

constexpr s32 ORBIS_CAMERA_MAX_DEVICE_NUM = 2;
constexpr s32 ORBIS_CAMERA_MAX_FORMAT_LEVEL_NUM = 4;
constexpr s32 ORBIS_CAMERA_CALIBRATION_DATA_MESH_NUMBER_OF_TOTAL_VERTICIES = 1276;

enum OrbisCameraChannel {
    ORBIS_CAMERA_CHANNEL_0 = 1,
    ORBIS_CAMERA_CHANNEL_1 = 2,
    ORBIS_CAMERA_CHANNEL_BOTH = 3,
};

struct OrbisCameraOpenParameter {
    u32 sizeThis;
    u32 reserved1;
    u32 reserved2;
    u32 reserved3;
};

enum OrbisCameraConfigType {
    ORBIS_CAMERA_CONFIG_TYPE1 = 0x01,
    ORBIS_CAMERA_CONFIG_TYPE2 = 0x02,
    ORBIS_CAMERA_CONFIG_TYPE3 = 0x03,
    ORBIS_CAMERA_CONFIG_TYPE4 = 0x04,
    ORBIS_CAMERA_CONFIG_TYPE5 = 0x05,
    ORBIS_CAMERA_CONFIG_EXTENTION = 0x10,
};

enum OrbisCameraResolution {
    ORBIS_CAMERA_RESOLUTION_1280X800 = 0x0,
    ORBIS_CAMERA_RESOLUTION_640X400 = 0x1,
    ORBIS_CAMERA_RESOLUTION_320X200 = 0x2,
    ORBIS_CAMERA_RESOLUTION_160X100 = 0x3,
    ORBIS_CAMERA_RESOLUTION_320X192 = 0x4,
    ORBIS_CAMERA_RESOLUTION_SPECIFIED_WIDTH_HEIGHT,
    ORBIS_CAMERA_RESOLUTION_UNKNOWN = 0xFF,
};

enum OrbisCameraFramerate {
    ORBIS_CAMERA_FRAMERATE_UNKNOWN = 0,
    ORBIS_CAMERA_FRAMERATE_7_5 = 7,
    ORBIS_CAMERA_FRAMERATE_15 = 15,
    ORBIS_CAMERA_FRAMERATE_30 = 30,
    ORBIS_CAMERA_FRAMERATE_60 = 60,
    ORBIS_CAMERA_FRAMERATE_120 = 120,
    ORBIS_CAMERA_FRAMERATE_240 = 240,
};

enum OrbisCameraBaseFormat {
    ORBIS_CAMERA_FORMAT_YUV422 = 0x0,
    ORBIS_CAMERA_FORMAT_RAW16,
    ORBIS_CAMERA_FORMAT_RAW8,
    ORBIS_CAMERA_FORMAT_NO_USE = 0x10,
    ORBIS_CAMERA_FORMAT_UNKNOWN = 0xFF,
};

enum OrbisCameraScaleFormat {
    ORBIS_CAMERA_SCALE_FORMAT_YUV422 = 0x0,
    ORBIS_CAMERA_SCALE_FORMAT_Y16 = 0x3,
    ORBIS_CAMERA_SCALE_FORMAT_Y8,
    ORBIS_CAMERA_SCALE_FORMAT_NO_USE = 0x10,
    ORBIS_CAMERA_SCALE_FORMAT_UNKNOWN = 0xFF,
};

enum OrbisCameraCalibrationDataFunctionType {
    ORBIS_CAMERA_CALIBRATION_DATA_FUNCTION_TYPE_IMAGE_RECTIFICATION = 0,
    ORBIS_CAMERA_CALIBRATION_DATA_FUNCTION_TYPE_IMAGE_INVERSE_RECTIFICATION = 1,
};

struct OrbisCameraFormat {
    OrbisCameraBaseFormat formatLevel0;
    OrbisCameraScaleFormat formatLevel1;
    OrbisCameraScaleFormat formatLevel2;
    OrbisCameraScaleFormat formatLevel3;
};

struct OrbisCameraConfigExtention {
    OrbisCameraFormat format;
    OrbisCameraResolution resolution;
    OrbisCameraFramerate framerate;
    u32 width;
    u32 height;
    u32 reserved1;
    void* pBaseOption;
};

struct OrbisCameraConfig {
    u32 sizeThis;
    OrbisCameraConfigType configType;
    OrbisCameraConfigExtention configExtention[ORBIS_CAMERA_MAX_DEVICE_NUM];
};

enum OrbisCameraAecAgcTarget {
    ORBIS_CAMERA_ATTRIBUTE_AECAGC_TARGET_DEF = 0x00,
    ORBIS_CAMERA_ATTRIBUTE_AECAGC_TARGET_2_0 = 0x20,
    ORBIS_CAMERA_ATTRIBUTE_AECAGC_TARGET_1_6 = 0x16,
    ORBIS_CAMERA_ATTRIBUTE_AECAGC_TARGET_1_4 = 0x14,
    ORBIS_CAMERA_ATTRIBUTE_AECAGC_TARGET_1_2 = 0x12,
    ORBIS_CAMERA_ATTRIBUTE_AECAGC_TARGET_1_0 = 0x10,
    ORBIS_CAMERA_ATTRIBUTE_AECAGC_TARGET_0_8 = 0x08,
    ORBIS_CAMERA_ATTRIBUTE_AECAGC_TARGET_0_6 = 0x06,
    ORBIS_CAMERA_ATTRIBUTE_AECAGC_TARGET_0_4 = 0x04,
    ORBIS_CAMERA_ATTRIBUTE_AECAGC_TARGET_0_2 = 0x02,
};

struct OrbisCameraDeviceInfo {
    u32 sizeThis;
    u32 infoRevision;
    u32 deviceRevision;
    u32 padding;
};

struct OrbisCameraStartParameter {
    u32 sizeThis;
    u32 formatLevel[ORBIS_CAMERA_MAX_DEVICE_NUM];
    void* pStartOption;
};

struct OrbisCameraVideoSyncParameter {
    u32 sizeThis;
    u32 videoSyncMode;
    void* pModeOption;
};

struct OrbisCameraFramePosition {
    u32 x;
    u32 y;
    u32 xSize;
    u32 ySize;
};

struct OrbisCameraAutoExposureGainTarget {
    u32 sizeThis;
    OrbisCameraAecAgcTarget target;
};

struct OrbisCameraExposureGain {
    u32 exposureControl;
    u32 exposure;
    u32 gain;
    u32 mode;
};

struct OrbisCameraWhiteBalance {
    u32 whiteBalanceControl;
    u32 gainRed;
    u32 gainBlue;
    u32 gainGreen;
};

struct OrbisCameraGamma {
    u32 gammaControl;
    u32 value;
    u8 reserved[16];
};

struct OrbisCameraMeta {
    u32 metaMode;
    u32 format[ORBIS_CAMERA_MAX_DEVICE_NUM][ORBIS_CAMERA_MAX_FORMAT_LEVEL_NUM];
    u64 frame[ORBIS_CAMERA_MAX_DEVICE_NUM];
    u64 timestamp[ORBIS_CAMERA_MAX_DEVICE_NUM];
    u32 deviceTimestamp[ORBIS_CAMERA_MAX_DEVICE_NUM];
    OrbisCameraExposureGain exposureGain[ORBIS_CAMERA_MAX_DEVICE_NUM];
    OrbisCameraWhiteBalance whiteBalance[ORBIS_CAMERA_MAX_DEVICE_NUM];
    OrbisCameraGamma gamma[ORBIS_CAMERA_MAX_DEVICE_NUM];
    u32 luminance[ORBIS_CAMERA_MAX_DEVICE_NUM];
    float acceleration_x;
    float acceleration_y;
    float acceleration_z;
    u64 vcounter;
    u32 reserved[14];
};

struct OrbisCameraFrameData {
    u32 sizeThis;
    u32 readMode;
    OrbisCameraFramePosition framePosition[ORBIS_CAMERA_MAX_DEVICE_NUM]
                                          [ORBIS_CAMERA_MAX_FORMAT_LEVEL_NUM];
    void* pFramePointerList[ORBIS_CAMERA_MAX_DEVICE_NUM][ORBIS_CAMERA_MAX_FORMAT_LEVEL_NUM];
    u32 frameSize[ORBIS_CAMERA_MAX_DEVICE_NUM][ORBIS_CAMERA_MAX_FORMAT_LEVEL_NUM];
    u32 status[ORBIS_CAMERA_MAX_DEVICE_NUM];
    OrbisCameraMeta meta;
    void* pFramePointerListGarlic[ORBIS_CAMERA_MAX_DEVICE_NUM][ORBIS_CAMERA_MAX_FORMAT_LEVEL_NUM];
};

struct OrbisCameraAttribute {
    u32 sizeThis;
    OrbisCameraChannel channel;
    OrbisCameraFramePosition framePosition;
    OrbisCameraExposureGain exposureGain;
    OrbisCameraWhiteBalance whiteBalance;
    OrbisCameraGamma gamma;
    u32 saturation;
    u32 contrast;
    u32 sharpness;
    s32 hue;
    u32 reserved1;
    u32 reserved2;
    u32 reserved3;
    u32 reserved4;
};

struct OrbisCameraGetCalibrationDataParameter {
    u32 size;
    u32 format_type;
    OrbisCameraCalibrationDataFunctionType function_type;
};

struct OrbisCameraCalibrationDataElement {
    u32 total_horizontal_verticies;
    u32 total_vertical_verticies;
    u32 left_margin;
    u32 right_margin;
    u32 top_margin;
    u32 bottom_margin;
    float x_table[ORBIS_CAMERA_CALIBRATION_DATA_MESH_NUMBER_OF_TOTAL_VERTICIES];
    float y_table[ORBIS_CAMERA_CALIBRATION_DATA_MESH_NUMBER_OF_TOTAL_VERTICIES];
};

struct OrbisCameraCalibrationData {
    u32 format_type;
    OrbisCameraCalibrationDataFunctionType function_type;
    OrbisCameraCalibrationDataElement data[2];
};

s32 PS4_SYSV_ABI sceCameraAccGetData();
s32 PS4_SYSV_ABI sceCameraAudioClose();
s32 PS4_SYSV_ABI sceCameraAudioGetData(void* data);
s32 PS4_SYSV_ABI sceCameraAudioGetData2(void* data);
s32 PS4_SYSV_ABI sceCameraAudioOpen();
s32 PS4_SYSV_ABI sceCameraAudioReset();
s32 PS4_SYSV_ABI sceCameraChangeAppModuleState();
s32 PS4_SYSV_ABI sceCameraClose(s32 handle);
s32 PS4_SYSV_ABI sceCameraCloseByHandle(s32 handle);
s32 PS4_SYSV_ABI sceCameraDeviceOpen();
s32 PS4_SYSV_ABI sceCameraGetAttribute(s32 handle, OrbisCameraAttribute* attribute);
s32 PS4_SYSV_ABI sceCameraGetAutoExposureGain(s32 handle, OrbisCameraChannel channel, u32* enable,
                                              OrbisCameraAutoExposureGainTarget* option);
s32 PS4_SYSV_ABI sceCameraGetAutoWhiteBalance(s32 handle, OrbisCameraChannel channel, u32* enable,
                                              void* option);
s32 PS4_SYSV_ABI sceCameraGetCalibData();
s32 PS4_SYSV_ABI sceCameraGetCalibDataFromDevice();
s32 PS4_SYSV_ABI sceCameraGetCalibrationData(const OrbisCameraGetCalibrationDataParameter* param,
                                             OrbisCameraCalibrationData* calibration_data);
s32 PS4_SYSV_ABI sceCameraGetConfig(s32 handle, OrbisCameraConfig* config);
s32 PS4_SYSV_ABI sceCameraGetContrast(s32 handle, OrbisCameraChannel channel, u32* contrast,
                                      void* option);
s32 PS4_SYSV_ABI sceCameraGetDefectivePixelCancellation(s32 handle, OrbisCameraChannel channel,
                                                        u32* enable, void* option);
s32 PS4_SYSV_ABI sceCameraGetDeviceConfig(s32 handle, OrbisCameraConfig* config);
s32 PS4_SYSV_ABI sceCameraGetDeviceConfigWithoutHandle(OrbisCameraConfig* config);
s32 PS4_SYSV_ABI sceCameraGetDeviceID();
s32 PS4_SYSV_ABI sceCameraGetDeviceIDWithoutOpen();
s32 PS4_SYSV_ABI sceCameraGetDeviceInfo(s32 reserved, OrbisCameraDeviceInfo* device_info);
s32 PS4_SYSV_ABI sceCameraGetExposureGain(s32 handle, OrbisCameraChannel channel,
                                          OrbisCameraExposureGain* exposure_gain, void* option);
s32 PS4_SYSV_ABI sceCameraGetFrameData(s32 handle, OrbisCameraFrameData* frame_data);
s32 PS4_SYSV_ABI sceCameraGetGamma(s32 handle, OrbisCameraChannel channel, OrbisCameraGamma* gamma,
                                   void* option);
s32 PS4_SYSV_ABI sceCameraGetHue(s32 handle, OrbisCameraChannel channel, s32* hue, void* option);
s32 PS4_SYSV_ABI sceCameraGetLensCorrection(s32 handle, OrbisCameraChannel channel, u32* enable,
                                            void* option);
s32 PS4_SYSV_ABI sceCameraGetMmapConnectedCount(u32* count);
s32 PS4_SYSV_ABI sceCameraGetProductInfo(void* product_info);
s32 PS4_SYSV_ABI sceCameraGetRegister();
s32 PS4_SYSV_ABI sceCameraGetRegistryInfo(void* registry_info);
s32 PS4_SYSV_ABI sceCameraGetSaturation(s32 handle, OrbisCameraChannel channel, u32* saturation,
                                        void* option);
s32 PS4_SYSV_ABI sceCameraGetSharpness(s32 handle, OrbisCameraChannel channel, u32* sharpness,
                                       void* option);
s32 PS4_SYSV_ABI sceCameraGetVrCaptureInfo();
s32 PS4_SYSV_ABI sceCameraGetWhiteBalance(s32 handle, OrbisCameraChannel channel,
                                          OrbisCameraWhiteBalance* white_balance, void* option);
s32 PS4_SYSV_ABI sceCameraInitializeRegistryCalibData();
s32 PS4_SYSV_ABI sceCameraIsAttached(s32 index);
s32 PS4_SYSV_ABI sceCameraIsConfigChangeDone();
s32 PS4_SYSV_ABI sceCameraIsValidFrameData(s32 handle, OrbisCameraFrameData* frame_data);
s32 PS4_SYSV_ABI sceCameraOpen(Libraries::UserService::OrbisUserServiceUserId user_id, s32 type,
                               s32 index, OrbisCameraOpenParameter* param);
s32 PS4_SYSV_ABI sceCameraOpenByModuleId();
s32 PS4_SYSV_ABI sceCameraRemoveAppModuleFocus();
s32 PS4_SYSV_ABI sceCameraSetAppModuleFocus();
s32 PS4_SYSV_ABI sceCameraSetAttribute(s32 handle, OrbisCameraAttribute* attribute);
s32 PS4_SYSV_ABI sceCameraSetAttributeInternal();
s32 PS4_SYSV_ABI sceCameraSetAutoExposureGain(s32 handle, OrbisCameraChannel channel, u32 enable,
                                              OrbisCameraAutoExposureGainTarget* option);
s32 PS4_SYSV_ABI sceCameraSetAutoWhiteBalance(s32 handle, OrbisCameraChannel channel, u32 enable,
                                              void* option);
s32 PS4_SYSV_ABI sceCameraSetCalibData();
s32 PS4_SYSV_ABI sceCameraSetConfig(s32 handle, OrbisCameraConfig* config);
s32 PS4_SYSV_ABI sceCameraSetConfigInternal(s32 handle, OrbisCameraConfig* config);
s32 PS4_SYSV_ABI sceCameraSetContrast(s32 handle, OrbisCameraChannel channel, u32 contrast,
                                      void* option);
s32 PS4_SYSV_ABI sceCameraSetDebugStop(u32 debug_stop_enable);
s32 PS4_SYSV_ABI sceCameraSetDefectivePixelCancellation(s32 handle, OrbisCameraChannel channel,
                                                        u32 enable, void* option);
s32 PS4_SYSV_ABI sceCameraSetDefectivePixelCancellationInternal(s32 handle,
                                                                OrbisCameraChannel channel,
                                                                u32 enable, void* option);
s32 PS4_SYSV_ABI sceCameraSetExposureGain(s32 handle, OrbisCameraChannel channel,
                                          OrbisCameraExposureGain* exposure_gain, void* option);
s32 PS4_SYSV_ABI sceCameraSetForceActivate();
s32 PS4_SYSV_ABI sceCameraSetGamma(s32 handle, OrbisCameraChannel channel, OrbisCameraGamma* gamma,
                                   void* option);
s32 PS4_SYSV_ABI sceCameraSetHue(s32 handle, OrbisCameraChannel channel, s32 hue, void* option);
s32 PS4_SYSV_ABI sceCameraSetLensCorrection(s32 handle, OrbisCameraChannel channel, u32 enable,
                                            void* option);
s32 PS4_SYSV_ABI sceCameraSetLensCorrectionInternal(s32 handle, OrbisCameraChannel channel,
                                                    u32 enable, void* option);
s32 PS4_SYSV_ABI sceCameraSetProcessFocus();
s32 PS4_SYSV_ABI sceCameraSetProcessFocusByHandle();
s32 PS4_SYSV_ABI sceCameraSetRegister();
s32 PS4_SYSV_ABI sceCameraSetSaturation(s32 handle, OrbisCameraChannel channel, u32 saturation,
                                        void* option);
s32 PS4_SYSV_ABI sceCameraSetSharpness(s32 handle, OrbisCameraChannel channel, u32 sharpness,
                                       void* option);
s32 PS4_SYSV_ABI sceCameraSetTrackerMode();
s32 PS4_SYSV_ABI sceCameraSetUacModeInternal();
s32 PS4_SYSV_ABI sceCameraSetVideoSync(s32 handle, OrbisCameraVideoSyncParameter* video_sync);
s32 PS4_SYSV_ABI sceCameraSetVideoSyncInternal(s32 handle,
                                               OrbisCameraVideoSyncParameter* video_sync);
s32 PS4_SYSV_ABI sceCameraSetWhiteBalance(s32 handle, OrbisCameraChannel channel,
                                          OrbisCameraWhiteBalance* white_balance, void* option);
s32 PS4_SYSV_ABI sceCameraStart(s32 handle, OrbisCameraStartParameter* param);
s32 PS4_SYSV_ABI sceCameraStartByHandle(s32 handle, OrbisCameraStartParameter* param);
s32 PS4_SYSV_ABI sceCameraStop(s32 handle);
s32 PS4_SYSV_ABI sceCameraStopByHandle(s32 handle);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Camera