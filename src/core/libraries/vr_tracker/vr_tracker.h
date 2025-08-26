// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/camera/camera.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::VrTracker {

static constexpr s32 ORBIS_VR_TRACKER_MAX_LED_NUM = 16;
static constexpr u32 ORBIS_VR_TRACKER_MEMORY_ALIGNMENT = 0x10000;
static constexpr u32 ORBIS_VR_TRACKER_BASE_ONION_SIZE = 0x400000;
static constexpr u32 ORBIS_VR_TRACKER_GARLIC_SIZE = 0x3000000;
static constexpr u32 ORBIS_VR_TRACKER_WORK_SIZE = 0x1000000;

enum OrbisVrTrackerProfile {
    ORBIS_VR_TRACKER_PROFILE_000 = 0,
    ORBIS_VR_TRACKER_PROFILE_100 = 100,
};

enum OrbisVrTrackerCalibrationMode {
    ORBIS_VR_TRACKER_CALIBRATION_MANUAL = 0,
    ORBIS_VR_TRACKER_CALIBRATION_AUTO = 1,
};

enum OrbisVrTrackerExecutionMode {
    ORBIS_VR_TRACKER_EXECUTION_MODE_SERIAL = 0,
    ORBIS_VR_TRACKER_EXECUTION_MODE_PARALLEL = 1,
};

enum OrbisVrTrackerDeviceType {
    ORBIS_VR_TRACKER_DEVICE_HMD = 0,
    ORBIS_VR_TRACKER_DEVICE_DUALSHOCK4 = 1,
    ORBIS_VR_TRACKER_DEVICE_MOVE = 2,
    ORBIS_VR_TRACKER_DEVICE_GUN = 3,
};

enum OrbisVrTrackerCalibrationType {
    ORBIS_VR_TRACKER_CALIBRATION_POSITION = 0,
    ORBIS_VR_TRACKER_CALIBRATION_ALL = 2,
};

enum OrbisVrTrackerPreferenceType {
    ORBIS_VR_TRACKER_PREFERENCE_FAR_POSITION = 0,
    ORBIS_VR_TRACKER_PREFERENCE_STABLE_POSITION = 1,
};

enum OrbisVrTrackerCameraMetaCheckMode {
    ORBIS_VR_TRACKER_CAMERA_META_CHECK_ENABLE = 0,
    ORBIS_VR_TRACKER_CAMERA_META_CHECK_DISABLE = 1,
};

enum OrbisVrTrackerDevicePermitType {
    ORBIS_VR_TRACKER_DEVICE_PERMIT_ALL = 0,
    ORBIS_VR_TRACKER_DEVICE_PERMIT_HMD_ONLY = 1,
};

enum OrbisVrTrackerRobustnessLevel {
    ORBIS_VR_TRACKER_ROBUSTNESS_LEVEL_HIGH = 0,
    ORBIS_VR_TRACKER_ROBUSTNESS_LEVEL_LOW = 3,
    ORBIS_VR_TRACKER_ROBUSTNESS_LEVEL_MEDIUM = 6,
    ORBIS_VR_TRACKER_ROBUSTNESS_LEVEL_LEGACY = 99,
};

enum OrbisVrTrackerCpuProcessOperationMode {
    ORBIS_VR_TRACKER_CPU_PROCESS_OPERATION_MODE_WHOLE = 0,
    ORBIS_VR_TRACKER_CPU_PROCESS_OPERATION_MODE_HANDLE = 1,
};

enum OrbisVrTrackerUpdateMotionSensorDataOperationMode {
    ORBIS_VR_TRACKER_UPDATE_MOTION_SENSORDATA_OPERATION_MODE_DEVICE_TYPE = 0,
    ORBIS_VR_TRACKER_UPDATE_MOTION_SENSORDATA_OPERATION_MODE_HANDLE = 1,
};

enum OrbisVrTrackerResultType {
    ORBIS_VR_TRACKER_RESULT_PREDICTED = 0,
    ORBIS_VR_TRACKER_RESULT_RAW = 1,
};

enum OrbisVrTrackerOrientationType {
    ORBIS_VR_TRACKER_ORIENTATION_ABSOLUTE = 0,
    ORBIS_VR_TRACKER_ORIENTATION_RELATIVE = 1,
};

enum OrbisVrTrackerUsageType {
    ORBIS_VR_TRACKER_USAGE_DEFAULT = 0,
    ORBIS_VR_TRACKER_USAGE_OPTIMIZED_FOR_HMD_USER = 1,
};

enum OrbisVrTrackerDebugMarkerType {
    ORBIS_VR_TRACKER_DEBUG_MARKER_UNSPECIFIED = 0,
    ORBIS_VR_TRACKER_DEBUG_MARKER_DEFAULT_UPDATE = 1,
    ORBIS_VR_TRACKER_DEBUG_MARKER_FINAL_UPDATE = 2,
    ORBIS_VR_TRACKER_DEBUG_MARKER_OTHER = 3,
};

enum OrbisVrTrackerRecalibrateNecessityType {
    ORBIS_VR_TRACKER_RECALIBRATE_NECESSITY_NOTHING = 0,
    ORBIS_VR_TRACKER_RECALIBRATE_NECESSITY_POSITION = 4,
};

enum OrbisVrTrackerPlayareaBrightnessRiskType {
    ORBIS_VR_TRACKER_PLAYAREA_BRIGHTNESS_RISK_LOW = 0,
    ORBIS_VR_TRACKER_PLAYAREA_BRIGHTNESS_RISK_HIGH = 5,
    ORBIS_VR_TRACKER_PLAYAREA_BRIGHTNESS_RISK_MAX = 10,
};

enum OrbisVrTrackerLedColor {
    ORBIS_VR_TRACKER_LED_COLOR_BLUE = 0,
    ORBIS_VR_TRACKER_LED_COLOR_RED = 1,
    ORBIS_VR_TRACKER_LED_COLOR_CYAN = 2,
    ORBIS_VR_TRACKER_LED_COLOR_MAGENTA = 3,
    ORBIS_VR_TRACKER_LED_COLOR_YELLOW = 4,
    ORBIS_VR_TRACKER_LED_COLOR_GREEN = 2
};

enum OrbisVrTrackerStatus {
    ORBIS_VR_TRACKER_STATUS_NOT_STARTED = 0,
    ORBIS_VR_TRACKER_STATUS_TRACKING = 1,
    ORBIS_VR_TRACKER_STATUS_NOT_TRACKING = 2,
    ORBIS_VR_TRACKER_STATUS_CALIBRATING = 3,
};

enum OrbisVrTrackerQuality {
    ORBIS_VR_TRACKER_QUALITY_NONE = 0,
    ORBIS_VR_TRACKER_QUALITY_NOT_VISIBLE = 3,
    ORBIS_VR_TRACKER_QUALITY_PARTIAL = 6,
    ORBIS_VR_TRACKER_QUALITY_FULL = 9,
};

enum OrbisVrTrackerLedAdjustmentStatus {
    ORBIS_VR_TRACKER_LED_ADJUSTMENT_NOT_USED = 0,
    ORBIS_VR_TRACKER_LED_ADJUSTMENT_USED = 1,
};

enum OrbisVrTrackerHmdRearTrackingStatus {
    ORBIS_VR_TRACKER_REAR_TRACKING_NOT_READY = 0,
    ORBIS_VR_TRACKER_REAR_TRACKING_READY = 1,
};

struct OrbisVrTrackerCalibrationSettings {
    OrbisVrTrackerCalibrationMode hmd_position;
    OrbisVrTrackerCalibrationMode pad_position;
    OrbisVrTrackerCalibrationMode move_position;
    OrbisVrTrackerCalibrationMode gun_position;
    u32 reserved[4];
};

struct OrbisVrTrackerQueryMemoryParam {
    u32 size;
    OrbisVrTrackerProfile profile;
    u32 reserved[6];
    OrbisVrTrackerCalibrationSettings calibration_settings;
};

struct OrbisVrTrackerQueryMemoryResult {
    u32 size;
    u32 direct_memory_onion_size;
    u32 direct_memory_onion_alignment;
    u32 direct_memory_garlic_size;
    u32 direct_memory_garlic_alignment;
    u32 work_memory_size;
    u32 work_memory_alignment;
    u32 reserved[9];
};

struct OrbisVrTrackerInitParam {
    u32 size;
    OrbisVrTrackerProfile profile;
    OrbisVrTrackerExecutionMode execution_mode;
    s32 hmd_thread_priority;
    s32 pad_thread_priority;
    s32 move_thread_priority;
    s32 gun_thread_priority;
    s32 reserved;
    u64 cpu_mask;
    OrbisVrTrackerCalibrationSettings calibration_settings;
    void* direct_memory_onion;
    u32 direct_memory_onion_size;
    u32 direct_memory_onion_alignment;
    void* direct_memory_garlic;
    u32 direct_memory_garlic_size;
    u32 direct_memory_garlic_alignment;
    void* work_memory;
    u32 work_memory_size;
    u32 work_memory_alignment;
    s32 gpu_pipe_id;
    s32 gpu_queue_id;
};

struct OrbisVrTrackerRecalibrateParam {
    u32 size;
    OrbisVrTrackerDeviceType device_type;
    OrbisVrTrackerCalibrationType calibration_type;
    u32 reserved[5];
};

struct OrbisVrTrackerGpuSubmitParam {
    u32 size;
    OrbisVrTrackerPreferenceType pad_tracking_preference;
    OrbisVrTrackerCameraMetaCheckMode camera_meta_check_mode;
    OrbisVrTrackerDevicePermitType tracking_device_permit_type;
    OrbisVrTrackerRobustnessLevel robustness_level;
    u32 reserved0[10];
    u32 reserved1;
    Libraries::Camera::OrbisCameraFrameData camera_frame_data;
};

struct OrbisVrTrackerGpuWaitParam {
    u32 size;
    u32 reserved[7];
};

struct OrbisVrTrackerCpuProcessParam {
    u32 size;
    OrbisVrTrackerCpuProcessOperationMode operation_mode;
    s32 handle;
    u32 reserved[5];
};

struct OrbisVrTrackerNotifyEndOfCpuProcessParam {
    u32 size;
    u32 reserved[7];
};

struct OrbisVrTrackerUpdateMotionSensorDataParam {
    u32 size;
    OrbisVrTrackerDeviceType device_type;
    OrbisVrTrackerUpdateMotionSensorDataOperationMode operation_mode;
    s32 handle;
    s32 reserved[4];
};

struct OrbisVrTrackerGetResultParam {
    u32 size;
    s32 handle;
    OrbisVrTrackerResultType result_type;
    u32 reserved0;
    u64 prediction_time;
    OrbisVrTrackerOrientationType orientation_type;
    u32 reserved1;
    OrbisVrTrackerUsageType usage_type;
    u32 user_frame_number;
    OrbisVrTrackerDebugMarkerType debug_marker_type;
    u32 reserved2[2];
};

struct OrbisVrTrackerPoseData {
    float position_x;
    float position_y;
    float position_z;
    u32 reserved0[1];
    float orientation_x;
    float orientation_y;
    float orientation_z;
    float orientation_w;
    u32 reserved1[8];
};

struct OrbisVrTrackerHmdInfo {
    OrbisVrTrackerPoseData device_pose;
    OrbisVrTrackerPoseData left_eye_pose;
    OrbisVrTrackerPoseData right_eye_pose;
    OrbisVrTrackerPoseData head_pose;
    OrbisVrTrackerHmdRearTrackingStatus rear_tracking_status;
    u32 reserved0[3];
    u64 sensor_read_system_timestamp;
    u32 reserved1[10];
};

struct OrbisVrTrackerPadInfo {
    OrbisVrTrackerPoseData device_pose;
    u32 reserved[64];
};

struct OrbisVrTrackerMoveInfo {
    OrbisVrTrackerPoseData device_pose;
    u32 reserved[64];
};

struct OrbisVrTrackerGunInfo {
    OrbisVrTrackerPoseData device_pose;
    u32 reserved[64];
};

struct OrbisVrTrackerLedResult {
    float x;
    float y;
    float rx;
    float ry;
    u32 reserved[4];
};

struct OrbisVrTrackerResultData {
    s32 handle;
    u32 connected;
    u32 reserved0[2];
    u64 timestamp;
    u64 device_timestamp;
    OrbisVrTrackerRecalibrateNecessityType recalibrate_necessity;
    OrbisVrTrackerPlayareaBrightnessRiskType playarea_brightness_risk;
    u32 reserved1[2];
    OrbisVrTrackerLedColor led_color;
    OrbisVrTrackerStatus status;
    OrbisVrTrackerQuality position_quality;
    OrbisVrTrackerQuality orientation_quality;
    float velocity_x;
    float velocity_y;
    float velocity_z;
    float acceleration_x;
    float acceleration_y;
    float acceleration_z;
    float angular_velocity_x;
    float angular_velocity_y;
    float angular_velocity_z;
    float angular_acceleration_x;
    float angular_acceleration_y;
    float angular_acceleration_z;
    float camera_orientation_x;
    float camera_orientation_y;
    float camera_orientation_z;
    float camera_orientation_w;
    union {
        OrbisVrTrackerHmdInfo hmd_info;
        OrbisVrTrackerPadInfo pad_info;
        OrbisVrTrackerMoveInfo move_info;
        OrbisVrTrackerGunInfo gun_info;
    };
    u32 user_frame_number;
    OrbisVrTrackerLedAdjustmentStatus led_adjustment_status;
    u64 timestamp_of_led_result;
    u32 reserved2[2];
    s32 number_of_led_result[Libraries::Camera::ORBIS_CAMERA_MAX_DEVICE_NUM];
    u32 reserved3[4];
    OrbisVrTrackerLedResult led[Libraries::Camera::ORBIS_CAMERA_MAX_DEVICE_NUM]
                               [ORBIS_VR_TRACKER_MAX_LED_NUM];
};

struct OrbisVrTrackerPlayAreaWarningInfo {
    u32 size;
    u32 reserved0[3];
    bool is_out_of_play_area;
    u8 reserved1[3];
    u32 reserved2[3];
    bool is_distance_data_valid;
    u8 reserved3[3];
    float distance_from_vertical_boundary;
    float distance_from_horizontal_boundary;
    u32 reserved4[5];
};

s32 PS4_SYSV_ABI sceVrTrackerQueryMemory(const OrbisVrTrackerQueryMemoryParam* param,
                                         OrbisVrTrackerQueryMemoryResult* result);
s32 PS4_SYSV_ABI sceVrTrackerInit(const OrbisVrTrackerInitParam* param);
s32 PS4_SYSV_ABI sceVrTrackerRegisterDevice(const OrbisVrTrackerDeviceType device_type,
                                            const s32 handle);
s32 PS4_SYSV_ABI sceVrTrackerRegisterDevice2(const OrbisVrTrackerDeviceType device_type,
                                             const s32 handle);
s32 PS4_SYSV_ABI sceVrTrackerRegisterDeviceInternal(const OrbisVrTrackerDeviceType device_type,
                                                    const s32 handle, s32 unk0, s32 unk1);
s32 PS4_SYSV_ABI sceVrTrackerCpuProcess(const OrbisVrTrackerCpuProcessParam* param);
s32 PS4_SYSV_ABI sceVrTrackerGetPlayAreaWarningInfo(OrbisVrTrackerPlayAreaWarningInfo* info);
s32 PS4_SYSV_ABI sceVrTrackerGetResult(const OrbisVrTrackerGetResultParam* param,
                                       OrbisVrTrackerResultData* result);
s32 PS4_SYSV_ABI sceVrTrackerGetTime(u64* time);
s32 PS4_SYSV_ABI sceVrTrackerGpuSubmit(const OrbisVrTrackerGpuSubmitParam* param);
s32 PS4_SYSV_ABI sceVrTrackerGpuWait(const OrbisVrTrackerGpuWaitParam* param);
s32 PS4_SYSV_ABI sceVrTrackerGpuWaitAndCpuProcess();
s32 PS4_SYSV_ABI
sceVrTrackerNotifyEndOfCpuProcess(const OrbisVrTrackerNotifyEndOfCpuProcessParam* param);
s32 PS4_SYSV_ABI sceVrTrackerRecalibrate(const OrbisVrTrackerRecalibrateParam* param);
s32 PS4_SYSV_ABI sceVrTrackerResetAll();
s32 PS4_SYSV_ABI sceVrTrackerResetOrientationRelative(const OrbisVrTrackerDeviceType device_type,
                                                      const s32 handle);
s32 PS4_SYSV_ABI sceVrTrackerSaveInternalBuffers();
s32 PS4_SYSV_ABI sceVrTrackerSetDurationUntilStatusNotTracking(
    const OrbisVrTrackerDeviceType device_type, const u32 duration_camera_frames);
s32 PS4_SYSV_ABI sceVrTrackerSetExtendedMode();
s32 PS4_SYSV_ABI sceVrTrackerSetLEDBrightness();
s32 PS4_SYSV_ABI sceVrTrackerSetRestingMode();
s32 PS4_SYSV_ABI
sceVrTrackerUpdateMotionSensorData(const OrbisVrTrackerUpdateMotionSensorDataParam* param);
s32 PS4_SYSV_ABI Func_0FA4C949F8D3024E();
s32 PS4_SYSV_ABI Func_285C6AFC09C42F7E();
s32 PS4_SYSV_ABI Func_9A6CDB2103664F8A();
s32 PS4_SYSV_ABI Func_B4D26B7D8B18DF06();
s32 PS4_SYSV_ABI sceVrTrackerSetDeviceRejection();
s32 PS4_SYSV_ABI Func_1119B0BE399F37E7();
s32 PS4_SYSV_ABI Func_4928B43816BC440D();
s32 PS4_SYSV_ABI Func_863EF32EFCB0FA9C();
s32 PS4_SYSV_ABI Func_E6E726CBC85C48F9();
s32 PS4_SYSV_ABI Func_F6407E46C66DF383();
s32 PS4_SYSV_ABI sceVrTrackerCpuPopMarker();
s32 PS4_SYSV_ABI sceVrTrackerCpuPushMarker();
s32 PS4_SYSV_ABI sceVrTrackerGetLiveCaptureId();
s32 PS4_SYSV_ABI sceVrTrackerStartLiveCapture();
s32 PS4_SYSV_ABI sceVrTrackerStopLiveCapture();
s32 PS4_SYSV_ABI sceVrTrackerUnregisterDevice(const s32 handle);
s32 PS4_SYSV_ABI sceVrTrackerTerm();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::VrTracker