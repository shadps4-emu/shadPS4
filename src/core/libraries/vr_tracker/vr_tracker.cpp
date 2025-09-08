// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/libs.h"
#include "core/libraries/vr_tracker/vr_tracker.h"
#include "core/libraries/vr_tracker/vr_tracker_error.h"
#include "core/memory.h"
#include "video_core/amdgpu/liverpool.h"

namespace Libraries::VrTracker {

static bool g_library_initialized = false;

// Internal memory
static void* g_garlic_memory_pointer = nullptr;
static u32 g_garlic_size = 0;
static void* g_onion_memory_pointer = nullptr;
static u32 g_onion_size = 0;
static void* g_work_memory_pointer = nullptr;
static u32 g_work_size = 0;

// Registered handles
static s32 g_pad_handle = -1;
static s32 g_move_handle = -1;
static s32 g_gun_handle = -1;
static s32 g_hmd_handle = -1;

s32 PS4_SYSV_ABI sceVrTrackerQueryMemory(const OrbisVrTrackerQueryMemoryParam* param,
                                         OrbisVrTrackerQueryMemoryResult* result) {
    LOG_DEBUG(Lib_VrTracker, "called");
    if (param == nullptr || result == nullptr ||
        param->size != sizeof(OrbisVrTrackerQueryMemoryParam) ||
        (param->profile != OrbisVrTrackerProfile::ORBIS_VR_TRACKER_PROFILE_000 &&
         param->profile != OrbisVrTrackerProfile::ORBIS_VR_TRACKER_PROFILE_100) ||
        param->calibration_settings.pad_position >
            OrbisVrTrackerCalibrationMode::ORBIS_VR_TRACKER_CALIBRATION_AUTO ||
        // Hmd doesn't support auto calibration
        param->calibration_settings.hmd_position >
            OrbisVrTrackerCalibrationMode::ORBIS_VR_TRACKER_CALIBRATION_MANUAL ||
        param->calibration_settings.move_position >
            OrbisVrTrackerCalibrationMode::ORBIS_VR_TRACKER_CALIBRATION_AUTO ||
        param->calibration_settings.gun_position >
            OrbisVrTrackerCalibrationMode::ORBIS_VR_TRACKER_CALIBRATION_AUTO) {
        return ORBIS_VR_TRACKER_ERROR_ARGUMENT_INVALID;
    }

    // Setting move_position to ORBIS_VR_TRACKER_CALIBRATION_AUTO doubles required onion memory.
    u32 required_onion_size = ORBIS_VR_TRACKER_BASE_ONION_SIZE;
    if (param->calibration_settings.move_position ==
        OrbisVrTrackerCalibrationMode::ORBIS_VR_TRACKER_CALIBRATION_AUTO) {
        required_onion_size *= 2;
    }

    result->direct_memory_onion_size = required_onion_size;
    result->direct_memory_onion_alignment = ORBIS_VR_TRACKER_MEMORY_ALIGNMENT;
    result->direct_memory_garlic_size = ORBIS_VR_TRACKER_GARLIC_SIZE;
    result->direct_memory_garlic_alignment = ORBIS_VR_TRACKER_MEMORY_ALIGNMENT;
    result->work_memory_size = ORBIS_VR_TRACKER_WORK_SIZE;
    result->work_memory_alignment = ORBIS_VR_TRACKER_MEMORY_ALIGNMENT;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerInit(const OrbisVrTrackerInitParam* param) {
    if (g_library_initialized) {
        return ORBIS_VR_TRACKER_ERROR_ALREADY_INITIALIZED;
    }

    // Calculate correct onion size for parameter checks
    u32 required_onion_size = ORBIS_VR_TRACKER_BASE_ONION_SIZE;
    if (param->calibration_settings.move_position == ORBIS_VR_TRACKER_CALIBRATION_AUTO) {
        required_onion_size *= 2;
    }

    // Parameter checks are fairly thorough here.
    if (param->size != sizeof(OrbisVrTrackerInitParam) ||
        // Check garlic memory parameters
        param->direct_memory_garlic == nullptr ||
        param->direct_memory_garlic_alignment != ORBIS_VR_TRACKER_MEMORY_ALIGNMENT ||
        param->direct_memory_garlic_size != ORBIS_VR_TRACKER_GARLIC_SIZE ||
        // Check onion memory parameters
        param->direct_memory_onion == nullptr ||
        param->direct_memory_onion_alignment != ORBIS_VR_TRACKER_MEMORY_ALIGNMENT ||
        param->direct_memory_onion_size != required_onion_size ||
        // Check work memory parameters
        param->work_memory == nullptr ||
        param->work_memory_alignment != ORBIS_VR_TRACKER_MEMORY_ALIGNMENT ||
        param->work_memory_size != ORBIS_VR_TRACKER_WORK_SIZE ||
        // Check compute queue parameters
        param->gpu_pipe_id >= AmdGpu::Liverpool::NumComputePipes ||
        param->gpu_queue_id >= AmdGpu::Liverpool::NumQueuesPerPipe ||
        // Check calibration settings
        param->calibration_settings.pad_position >
            OrbisVrTrackerCalibrationMode::ORBIS_VR_TRACKER_CALIBRATION_AUTO ||
        param->calibration_settings.hmd_position >
            OrbisVrTrackerCalibrationMode::ORBIS_VR_TRACKER_CALIBRATION_MANUAL ||
        param->calibration_settings.move_position >
            OrbisVrTrackerCalibrationMode::ORBIS_VR_TRACKER_CALIBRATION_AUTO ||
        param->calibration_settings.gun_position >
            OrbisVrTrackerCalibrationMode::ORBIS_VR_TRACKER_CALIBRATION_AUTO) {
        return ORBIS_VR_TRACKER_ERROR_ARGUMENT_INVALID;
    }

    // Real hardware will segfault if any of the supplied mappings aren't long enough,
    // Validate each of them to ensure nothing weird can occur when this library is implemented.
    auto* memory = Core::Memory::Instance();
    Libraries::Kernel::OrbisVirtualQueryInfo info;
    // The memory type for the whole range should be the same here,
    // so the memory should be contained in one VMA.
    VAddr addr_to_check = std::bit_cast<VAddr>(param->direct_memory_garlic);
    s32 result = memory->VirtualQuery(addr_to_check, 0, &info);
    ASSERT_MSG(result == 0 && info.end - addr_to_check >= param->direct_memory_garlic_size,
               "Insufficient garlic memory provided");

    g_garlic_memory_pointer = param->direct_memory_garlic;
    g_garlic_size = param->direct_memory_garlic_size;

    addr_to_check = std::bit_cast<VAddr>(param->direct_memory_onion);
    result = memory->VirtualQuery(addr_to_check, 0, &info);
    ASSERT_MSG(result == 0 && info.end - addr_to_check >= param->direct_memory_onion_size,
               "Insufficient onion memory provided");

    g_onion_memory_pointer = param->direct_memory_onion;
    g_onion_size = param->direct_memory_onion_size;

    addr_to_check = std::bit_cast<VAddr>(param->work_memory);
    result = memory->VirtualQuery(addr_to_check, 0, &info);
    ASSERT_MSG(result == 0 && info.end - addr_to_check >= param->work_memory_size,
               "Insufficient work memory provided");

    g_work_memory_pointer = param->work_memory;
    g_work_size = param->work_memory_size;

    // All initialization checks passed.
    LOG_WARNING(Lib_VrTracker, "PSVR headsets are not supported yet");
    g_library_initialized = true;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerRegisterDevice(const OrbisVrTrackerDeviceType device_type,
                                            const s32 handle) {
    LOG_TRACE(Lib_VrTracker, "redirected to sceVrTrackerRegisterDeviceInternal");
    return sceVrTrackerRegisterDeviceInternal(device_type, handle, -1, 0);
}

s32 PS4_SYSV_ABI sceVrTrackerRegisterDevice2(const OrbisVrTrackerDeviceType device_type,
                                             const s32 handle) {
    LOG_TRACE(Lib_VrTracker, "redirected to sceVrTrackerRegisterDeviceInternal");
    return sceVrTrackerRegisterDeviceInternal(device_type, handle, -1, 1);
}

s32 PS4_SYSV_ABI sceVrTrackerRegisterDeviceInternal(const OrbisVrTrackerDeviceType device_type,
                                                    const s32 handle, s32 unk0, s32 unk1) {
    LOG_WARNING(Lib_VrTracker, "(STUBBED) called, device_type = {}, handle = {}",
                static_cast<u32>(device_type), handle);
    if (!g_library_initialized) {
        return ORBIS_VR_TRACKER_ERROR_NOT_INIT;
    }
    if (device_type > OrbisVrTrackerDeviceType::ORBIS_VR_TRACKER_DEVICE_GUN || unk0 > 4) {
        return ORBIS_VR_TRACKER_ERROR_ARGUMENT_INVALID;
    }

    // Ignore handle handle validation for now, since most of that logic isn't really handled.
    switch (device_type) {
    case OrbisVrTrackerDeviceType::ORBIS_VR_TRACKER_DEVICE_HMD: {
        if (g_hmd_handle != -1) {
            return ORBIS_VR_TRACKER_ERROR_DEVICE_ALREADY_REGISTERED;
        }
        g_hmd_handle = handle;
        break;
    }
    case OrbisVrTrackerDeviceType::ORBIS_VR_TRACKER_DEVICE_DUALSHOCK4: {
        if (g_pad_handle != -1) {
            return ORBIS_VR_TRACKER_ERROR_DEVICE_ALREADY_REGISTERED;
        }
        g_pad_handle = handle;
        break;
    }
    case OrbisVrTrackerDeviceType::ORBIS_VR_TRACKER_DEVICE_MOVE: {
        if (g_move_handle != -1) {
            return ORBIS_VR_TRACKER_ERROR_DEVICE_ALREADY_REGISTERED;
        }
        g_move_handle = handle;
        break;
    }
    case OrbisVrTrackerDeviceType::ORBIS_VR_TRACKER_DEVICE_GUN: {
        if (g_gun_handle != -1) {
            return ORBIS_VR_TRACKER_ERROR_DEVICE_ALREADY_REGISTERED;
        }
        g_gun_handle = handle;
        break;
    }
    default: {
        // Shouldn't be possible to hit this.
        UNREACHABLE();
    }
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerCpuProcess(const OrbisVrTrackerCpuProcessParam* param) {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerGetPlayAreaWarningInfo(OrbisVrTrackerPlayAreaWarningInfo* info) {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerGetResult(const OrbisVrTrackerGetResultParam* param,
                                       OrbisVrTrackerResultData* result) {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerGetTime(u64* time) {
    LOG_TRACE(Lib_VrTracker, "called");
    if (!g_library_initialized) {
        return ORBIS_VR_TRACKER_ERROR_NOT_INIT;
    }
    if (time == nullptr) {
        return ORBIS_VR_TRACKER_ERROR_ARGUMENT_INVALID;
    }
    *time = Libraries::Kernel::sceKernelGetProcessTime();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerGpuSubmit(const OrbisVrTrackerGpuSubmitParam* param) {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    if (!g_library_initialized) {
        return ORBIS_VR_TRACKER_ERROR_NOT_INIT;
    }

    // Impossible to submit valid data here since sceCameraGetFrameData returns an error.
    return ORBIS_VR_TRACKER_ERROR_ARGUMENT_INVALID;
}

s32 PS4_SYSV_ABI sceVrTrackerGpuWait(const OrbisVrTrackerGpuWaitParam* param) {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    if (!g_library_initialized) {
        return ORBIS_VR_TRACKER_ERROR_NOT_INIT;
    }
    if (param == nullptr || param->size != sizeof(OrbisVrTrackerGpuWaitParam)) {
        return ORBIS_VR_TRACKER_ERROR_ARGUMENT_INVALID;
    }

    // Impossible to perform GPU submits
    return ORBIS_VR_TRACKER_ERROR_NOT_EXECUTE_GPU_SUBMIT;
}

s32 PS4_SYSV_ABI sceVrTrackerGpuWaitAndCpuProcess() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    if (!g_library_initialized) {
        return ORBIS_VR_TRACKER_ERROR_NOT_INIT;
    }

    // Impossible to perform GPU submits
    return ORBIS_VR_TRACKER_ERROR_NOT_EXECUTE_GPU_SUBMIT;
}

s32 PS4_SYSV_ABI
sceVrTrackerNotifyEndOfCpuProcess(const OrbisVrTrackerNotifyEndOfCpuProcessParam* param) {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerRecalibrate(const OrbisVrTrackerRecalibrateParam* param) {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    if (!g_library_initialized) {
        return ORBIS_VR_TRACKER_ERROR_NOT_INIT;
    }
    if (param == nullptr || param->size != sizeof(OrbisVrTrackerRecalibrateParam) ||
        param->device_type > OrbisVrTrackerDeviceType::ORBIS_VR_TRACKER_DEVICE_GUN) {
        return ORBIS_VR_TRACKER_ERROR_ARGUMENT_INVALID;
    }

    OrbisVrTrackerDeviceType device_type = param->device_type;
    switch (device_type) {
    case OrbisVrTrackerDeviceType::ORBIS_VR_TRACKER_DEVICE_HMD: {
        // Seems like the lack of a connected hmd results in this?
        return ORBIS_VR_TRACKER_ERROR_DEVICE_NOT_REGISTERED;
        break;
    }
    case OrbisVrTrackerDeviceType::ORBIS_VR_TRACKER_DEVICE_DUALSHOCK4: {
        if (g_pad_handle == -1) {
            return ORBIS_VR_TRACKER_ERROR_DEVICE_NOT_REGISTERED;
        }
        break;
    }
    case OrbisVrTrackerDeviceType::ORBIS_VR_TRACKER_DEVICE_MOVE: {
        if (g_move_handle == -1) {
            return ORBIS_VR_TRACKER_ERROR_DEVICE_NOT_REGISTERED;
        }
        break;
    }
    case OrbisVrTrackerDeviceType::ORBIS_VR_TRACKER_DEVICE_GUN: {
        if (g_gun_handle == -1) {
            return ORBIS_VR_TRACKER_ERROR_DEVICE_NOT_REGISTERED;
        }
        break;
    }
    default: {
        // Shouldn't be possible to hit this.
        UNREACHABLE();
    }
    }

    // TODO: handle internal recalibration behaviors.
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerResetAll() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerResetOrientationRelative(const OrbisVrTrackerDeviceType device_type,
                                                      const s32 handle) {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerSaveInternalBuffers() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerSetDurationUntilStatusNotTracking(
    const OrbisVrTrackerDeviceType device_type, const u32 duration_camera_frames) {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    if (!g_library_initialized) {
        return ORBIS_VR_TRACKER_ERROR_NOT_INIT;
    }
    if (device_type > OrbisVrTrackerDeviceType::ORBIS_VR_TRACKER_DEVICE_GUN) {
        return ORBIS_VR_TRACKER_ERROR_ARGUMENT_INVALID;
    }

    // Seems to unconditionally return 0 when parameters are valid.
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerSetExtendedMode() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerSetLEDBrightness() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerSetRestingMode() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceVrTrackerUpdateMotionSensorData(const OrbisVrTrackerUpdateMotionSensorDataParam* param) {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_0FA4C949F8D3024E() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_285C6AFC09C42F7E() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_9A6CDB2103664F8A() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_B4D26B7D8B18DF06() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerSetDeviceRejection() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_1119B0BE399F37E7() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_4928B43816BC440D() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_863EF32EFCB0FA9C() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_E6E726CBC85C48F9() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_F6407E46C66DF383() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerCpuPopMarker() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerCpuPushMarker() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerGetLiveCaptureId() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerStartLiveCapture() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerStopLiveCapture() {
    LOG_ERROR(Lib_VrTracker, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerUnregisterDevice(const s32 handle) {
    LOG_DEBUG(Lib_VrTracker, "called");
    if (!g_library_initialized) {
        return ORBIS_VR_TRACKER_ERROR_NOT_INIT;
    }
    if (handle < 0) {
        return ORBIS_VR_TRACKER_ERROR_ARGUMENT_INVALID;
    }
    // Since this function only takes a handle, compare the handle to registered handles.
    if (handle == g_hmd_handle) {
        g_hmd_handle = -1;
    } else if (handle == g_pad_handle) {
        g_pad_handle = -1;
    } else if (handle == g_move_handle) {
        g_move_handle = -1;
    } else if (handle == g_gun_handle) {
        g_gun_handle = -1;
    } else {
        // If none of the handles match up, then return an error.
        return ORBIS_VR_TRACKER_ERROR_ARGUMENT_INVALID;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrTrackerTerm() {
    LOG_DEBUG(Lib_VrTracker, "called");
    if (!g_library_initialized) {
        return ORBIS_VR_TRACKER_ERROR_NOT_INIT;
    }
    g_library_initialized = false;
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    // OpenOrbis Toolchain seems to link with module version 1,1 while actual games use 0,0
    LIB_FUNCTION("24kDA+A0Ox0", "libSceVrTrackerFourDeviceAllowed", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerRegisterDevice2);
    LIB_FUNCTION("5IFOAYv-62g", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerCpuProcess);
    LIB_FUNCTION("zvyKP0Z3UvU", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerGetPlayAreaWarningInfo);
    LIB_FUNCTION("76OBvrrQXUc", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerGetResult);
    LIB_FUNCTION("XoeWzXlrnMw", "libSceVrTracker", 1, "libSceVrTracker", 1, 1, sceVrTrackerGetTime);
    LIB_FUNCTION("TVegDMLaBB8", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerGpuSubmit);
    LIB_FUNCTION("gkGuO9dd57M", "libSceVrTracker", 1, "libSceVrTracker", 1, 1, sceVrTrackerGpuWait);
    LIB_FUNCTION("ARhgpXvwoR0", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerGpuWaitAndCpuProcess);
    LIB_FUNCTION("QkRl7pART9M", "libSceVrTracker", 1, "libSceVrTracker", 1, 1, sceVrTrackerInit);
    LIB_FUNCTION("VItTwN8DmS8", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerNotifyEndOfCpuProcess);
    LIB_FUNCTION("K7yhYrsIBPc", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerQueryMemory);
    LIB_FUNCTION("EUCaQtXXYNI", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerRecalibrate);
    LIB_FUNCTION("sIh8GwcevaQ", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerRegisterDevice);
    LIB_FUNCTION("ufexf4aNiwg", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerRegisterDeviceInternal);
    LIB_FUNCTION("CtWUbFgmq+I", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerResetAll);
    LIB_FUNCTION("E0P0sN-wy+4", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerResetOrientationRelative);
    LIB_FUNCTION("bDGZVTwwZ1A", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerSaveInternalBuffers);
    LIB_FUNCTION("qBjnR0HtMYI", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerSetDurationUntilStatusNotTracking);
    LIB_FUNCTION("NhPkY3V8E+8", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerSetExtendedMode);
    LIB_FUNCTION("vpsLLotiSUg", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerSetLEDBrightness);
    LIB_FUNCTION("lgWSHQ8p4i4", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerSetRestingMode);
    LIB_FUNCTION("IBv4P3q1pQ0", "libSceVrTracker", 1, "libSceVrTracker", 1, 1, sceVrTrackerTerm);
    LIB_FUNCTION("Q8skQqEwn5c", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerUnregisterDevice);
    LIB_FUNCTION("9fvHMUbsom4", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerUpdateMotionSensorData);
    LIB_FUNCTION("D6TJSfjTAk4", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 Func_0FA4C949F8D3024E);
    LIB_FUNCTION("KFxq-AnEL34", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 Func_285C6AFC09C42F7E);
    LIB_FUNCTION("mmzbIQNmT4o", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 Func_9A6CDB2103664F8A);
    LIB_FUNCTION("tNJrfYsY3wY", "libSceVrTracker", 1, "libSceVrTracker", 1, 1,
                 Func_B4D26B7D8B18DF06);
    LIB_FUNCTION("jGqEkPy0iLU", "libSceVrTrackerDeviceRejection", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerSetDeviceRejection);
    LIB_FUNCTION("ERmwvjmfN+c", "libSceVrTrackerGpuTest", 1, "libSceVrTracker", 1, 1,
                 Func_1119B0BE399F37E7);
    LIB_FUNCTION("SSi0OBa8RA0", "libSceVrTrackerGpuTest", 1, "libSceVrTracker", 1, 1,
                 Func_4928B43816BC440D);
    LIB_FUNCTION("hj7zLvyw+pw", "libSceVrTrackerGpuTest", 1, "libSceVrTracker", 1, 1,
                 Func_863EF32EFCB0FA9C);
    LIB_FUNCTION("5ucmy8hcSPk", "libSceVrTrackerGpuTest", 1, "libSceVrTracker", 1, 1,
                 Func_E6E726CBC85C48F9);
    LIB_FUNCTION("9kB+RsZt84M", "libSceVrTrackerGpuTest", 1, "libSceVrTracker", 1, 1,
                 Func_F6407E46C66DF383);
    LIB_FUNCTION("sBkAqyF5Gns", "libSceVrTrackerLiveCapture", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerCpuPopMarker);
    LIB_FUNCTION("rvCywCbc7Pk", "libSceVrTrackerLiveCapture", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerCpuPushMarker);
    LIB_FUNCTION("lm6T1Ur6JRk", "libSceVrTrackerLiveCapture", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerGetLiveCaptureId);
    LIB_FUNCTION("qa1+CeXKDPc", "libSceVrTrackerLiveCapture", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerStartLiveCapture);
    LIB_FUNCTION("3YCwwpHkHIg", "libSceVrTrackerLiveCapture", 1, "libSceVrTracker", 1, 1,
                 sceVrTrackerStopLiveCapture);

    // For proper games
    LIB_FUNCTION("24kDA+A0Ox0", "libSceVrTrackerFourDeviceAllowed", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerRegisterDevice2);
    LIB_FUNCTION("5IFOAYv-62g", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerCpuProcess);
    LIB_FUNCTION("zvyKP0Z3UvU", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerGetPlayAreaWarningInfo);
    LIB_FUNCTION("76OBvrrQXUc", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerGetResult);
    LIB_FUNCTION("XoeWzXlrnMw", "libSceVrTracker", 1, "libSceVrTracker", 0, 0, sceVrTrackerGetTime);
    LIB_FUNCTION("TVegDMLaBB8", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerGpuSubmit);
    LIB_FUNCTION("gkGuO9dd57M", "libSceVrTracker", 1, "libSceVrTracker", 0, 0, sceVrTrackerGpuWait);
    LIB_FUNCTION("ARhgpXvwoR0", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerGpuWaitAndCpuProcess);
    LIB_FUNCTION("QkRl7pART9M", "libSceVrTracker", 1, "libSceVrTracker", 0, 0, sceVrTrackerInit);
    LIB_FUNCTION("VItTwN8DmS8", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerNotifyEndOfCpuProcess);
    LIB_FUNCTION("K7yhYrsIBPc", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerQueryMemory);
    LIB_FUNCTION("EUCaQtXXYNI", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerRecalibrate);
    LIB_FUNCTION("sIh8GwcevaQ", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerRegisterDevice);
    LIB_FUNCTION("ufexf4aNiwg", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerRegisterDeviceInternal);
    LIB_FUNCTION("CtWUbFgmq+I", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerResetAll);
    LIB_FUNCTION("E0P0sN-wy+4", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerResetOrientationRelative);
    LIB_FUNCTION("bDGZVTwwZ1A", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerSaveInternalBuffers);
    LIB_FUNCTION("qBjnR0HtMYI", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerSetDurationUntilStatusNotTracking);
    LIB_FUNCTION("NhPkY3V8E+8", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerSetExtendedMode);
    LIB_FUNCTION("vpsLLotiSUg", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerSetLEDBrightness);
    LIB_FUNCTION("lgWSHQ8p4i4", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerSetRestingMode);
    LIB_FUNCTION("IBv4P3q1pQ0", "libSceVrTracker", 1, "libSceVrTracker", 0, 0, sceVrTrackerTerm);
    LIB_FUNCTION("Q8skQqEwn5c", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerUnregisterDevice);
    LIB_FUNCTION("9fvHMUbsom4", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerUpdateMotionSensorData);
    LIB_FUNCTION("D6TJSfjTAk4", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 Func_0FA4C949F8D3024E);
    LIB_FUNCTION("KFxq-AnEL34", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 Func_285C6AFC09C42F7E);
    LIB_FUNCTION("mmzbIQNmT4o", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 Func_9A6CDB2103664F8A);
    LIB_FUNCTION("tNJrfYsY3wY", "libSceVrTracker", 1, "libSceVrTracker", 0, 0,
                 Func_B4D26B7D8B18DF06);
    LIB_FUNCTION("jGqEkPy0iLU", "libSceVrTrackerDeviceRejection", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerSetDeviceRejection);
    LIB_FUNCTION("ERmwvjmfN+c", "libSceVrTrackerGpuTest", 1, "libSceVrTracker", 0, 0,
                 Func_1119B0BE399F37E7);
    LIB_FUNCTION("SSi0OBa8RA0", "libSceVrTrackerGpuTest", 1, "libSceVrTracker", 0, 0,
                 Func_4928B43816BC440D);
    LIB_FUNCTION("hj7zLvyw+pw", "libSceVrTrackerGpuTest", 1, "libSceVrTracker", 0, 0,
                 Func_863EF32EFCB0FA9C);
    LIB_FUNCTION("5ucmy8hcSPk", "libSceVrTrackerGpuTest", 1, "libSceVrTracker", 0, 0,
                 Func_E6E726CBC85C48F9);
    LIB_FUNCTION("9kB+RsZt84M", "libSceVrTrackerGpuTest", 1, "libSceVrTracker", 0, 0,
                 Func_F6407E46C66DF383);
    LIB_FUNCTION("sBkAqyF5Gns", "libSceVrTrackerLiveCapture", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerCpuPopMarker);
    LIB_FUNCTION("rvCywCbc7Pk", "libSceVrTrackerLiveCapture", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerCpuPushMarker);
    LIB_FUNCTION("lm6T1Ur6JRk", "libSceVrTrackerLiveCapture", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerGetLiveCaptureId);
    LIB_FUNCTION("qa1+CeXKDPc", "libSceVrTrackerLiveCapture", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerStartLiveCapture);
    LIB_FUNCTION("3YCwwpHkHIg", "libSceVrTrackerLiveCapture", 1, "libSceVrTracker", 0, 0,
                 sceVrTrackerStopLiveCapture);
};

} // namespace Libraries::VrTracker