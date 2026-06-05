// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/video_recording/video_recording.h"
#include "core/libraries/video_recording/video_recording_error.h"

namespace Libraries::VideoRecording {

s32 PS4_SYSV_ABI sceVideoRecordingSetInfo(OrbisVideoRecordingInfo set_info, const void* info,
                                          u64 info_len) {
    LOG_ERROR(Lib_VideoRecording, "(STUBBED) called, set_info = {:#x}, info_len = {:#x}",
              static_cast<u32>(set_info), info_len);
    if (set_info < OrbisVideoRecordingInfo::Subtitle ||
        (set_info > OrbisVideoRecordingInfo::Subtitle &&
         set_info < OrbisVideoRecordingInfo::Description) ||
        (set_info > OrbisVideoRecordingInfo::Keywords &&
         set_info < OrbisVideoRecordingInfo::Chapter) ||
        (set_info > OrbisVideoRecordingInfo::Chapter &&
         set_info < OrbisVideoRecordingInfo::Copyright) ||
        (set_info > OrbisVideoRecordingInfo::Chapter &&
         set_info < OrbisVideoRecordingInfo::PermisssionLevel) ||
        set_info > OrbisVideoRecordingInfo::UserMeta) {
        return ORBIS_VIDEO_RECORDING_ERROR_INVALID_VALUE;
    }

    // The library seems to rely on libSceAvcap to provide errors for other parameters.
    if (!info) {
        return ORBIS_VIDEO_RECORDING_ERROR_INVALID_VALUE;
    }
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("Fc8qxlKINYQ", "libSceVideoRecording", 1, "libSceVideoRecording",
                 sceVideoRecordingSetInfo);
}
} // namespace Libraries::VideoRecording