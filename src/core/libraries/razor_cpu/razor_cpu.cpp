// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "razor_cpu.h"

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::RazorCpu {

s32 PS4_SYSV_ABI sceRazorCpuBeginLogicalFileAccess() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceRazorCpuDisableFiberUserMarkers() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
}

s32 PS4_SYSV_ABI sceRazorCpuEndLogicalFileAccess() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuFiberLogNameChange() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuFiberSwitch() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

bool PS4_SYSV_ABI sceRazorCpuFlushOccurred() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return false;
}

s32 PS4_SYSV_ABI sceRazorCpuGetDataTagStorageSize() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuGpuMarkerSync() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuInitDataTags() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuInitializeGpuMarkerContext() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuInitializeInternal() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

bool PS4_SYSV_ABI sceRazorCpuIsCapturing() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return false;
}

s32 PS4_SYSV_ABI sceRazorCpuJobManagerDispatch() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuJobManagerJob() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuJobManagerSequence() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuNamedSync() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuPlotValue() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuPopMarker() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuPushMarker() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuPushMarkerStatic() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuResizeTaggedBuffer() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceRazorCpuSetPopMarkerCallback() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
}

void PS4_SYSV_ABI sceRazorCpuSetPushMarkerCallback() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
}

void PS4_SYSV_ABI sceRazorCpuSetPushMarkerStaticCallback() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
}

s32 PS4_SYSV_ABI sceRazorCpuShutdownDataTags() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuStartCaptureInternal() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuStopCaptureInternal() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuSync() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuTagArray() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuTagBuffer() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuUnTagBuffer() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuWorkloadRunBegin() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuWorkloadRunEnd() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuWorkloadSubmit() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceRazorCpuWriteBookmark() {
    LOG_DEBUG(Lib_RazorCpu, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("JFzLJBlYIJE", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuBeginLogicalFileAccess);
    LIB_FUNCTION("SfRTRZ1Sh+U", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuDisableFiberUserMarkers);
    LIB_FUNCTION("gVioM9cbiDs", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuEndLogicalFileAccess);
    LIB_FUNCTION("G90IIOtgFQ0", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuFiberLogNameChange);
    LIB_FUNCTION("PAytDtFGpqY", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuFiberSwitch);
    LIB_FUNCTION("sPhrQD31ClM", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuFlushOccurred);
    LIB_FUNCTION("B782NptkGUc", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuGetDataTagStorageSize);
    LIB_FUNCTION("EH9Au2RlSrE", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuGpuMarkerSync);
    LIB_FUNCTION("A7oRMdaOJP8", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuInitDataTags);
    LIB_FUNCTION("NFwh-J-BrI0", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuInitializeGpuMarkerContext);
    LIB_FUNCTION("ElNyedXaa4o", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuInitializeInternal);
    LIB_FUNCTION("EboejOQvLL4", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuIsCapturing);
    LIB_FUNCTION("dnEdyY4+klQ", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuJobManagerDispatch);
    LIB_FUNCTION("KP+TBWGHlgs", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuJobManagerJob);
    LIB_FUNCTION("9FowWFMEIM8", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuJobManagerSequence);
    LIB_FUNCTION("XCuZoBSVFG8", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1, sceRazorCpuNamedSync);
    LIB_FUNCTION("njGikRrxkC0", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1, sceRazorCpuPlotValue);
    LIB_FUNCTION("YpkGsMXP3ew", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1, sceRazorCpuPopMarker);
    LIB_FUNCTION("zw+celG7zSI", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1, sceRazorCpuPushMarker);
    LIB_FUNCTION("uZrOwuNJX-M", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuPushMarkerStatic);
    LIB_FUNCTION("D0yUjM33QqU", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuResizeTaggedBuffer);
    LIB_FUNCTION("jqYWaTfgZs0", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuSetPopMarkerCallback);
    LIB_FUNCTION("DJsHcEb94n0", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuSetPushMarkerCallback);
    LIB_FUNCTION("EZtqozPTS4M", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuSetPushMarkerStaticCallback);
    LIB_FUNCTION("emklx7RK-LY", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuShutdownDataTags);
    LIB_FUNCTION("TIytAjYeaik", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuStartCaptureInternal);
    LIB_FUNCTION("jWpkVWdMrsM", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuStopCaptureInternal);
    LIB_FUNCTION("Ax7NjOzctIM", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1, sceRazorCpuSync);
    LIB_FUNCTION("we3oTKSPSTw", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1, sceRazorCpuTagArray);
    LIB_FUNCTION("vyjdThnQfQQ", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1, sceRazorCpuTagBuffer);
    LIB_FUNCTION("0yNHPIkVTmw", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuUnTagBuffer);
    LIB_FUNCTION("Crha9LvwvJM", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuWorkloadRunBegin);
    LIB_FUNCTION("q1GxBfGHO0s", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuWorkloadRunEnd);
    LIB_FUNCTION("6rUvx-6QmYc", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuWorkloadSubmit);
    LIB_FUNCTION("G3brhegfyNg", "libSceRazorCpu", 1, "libSceRazorCpu", 1, 1,
                 sceRazorCpuWriteBookmark);
}

} // namespace Libraries::RazorCpu