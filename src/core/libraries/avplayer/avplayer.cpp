// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/avplayer/avplayer.h"
#include "core/libraries/avplayer/avplayer_error.h"
#include "core/libraries/avplayer/avplayer_impl.h"
#include "core/libraries/libs.h"

namespace Libraries::AvPlayer {

s32 PS4_SYSV_ABI sceAvPlayerAddSource(AvPlayerHandle handle, const char* filename) {
    LOG_TRACE(Lib_AvPlayer, "filename = {}", filename);
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return handle->AddSource(filename);
}

s32 PS4_SYSV_ABI sceAvPlayerAddSourceEx(AvPlayerHandle handle, AvPlayerUriType uri_type,
                                        AvPlayerSourceDetails* source_details) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr || uri_type != AvPlayerUriType::Source) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    const auto path = std::string_view(source_details->uri.name, source_details->uri.length);
    return handle->AddSourceEx(path, source_details->source_type);
}

int PS4_SYSV_ABI sceAvPlayerChangeStream() {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerClose(AvPlayerHandle handle) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    delete handle;
    return ORBIS_OK;
}

u64 PS4_SYSV_ABI sceAvPlayerCurrentTime(AvPlayerHandle handle) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return handle->CurrentTime();
}

s32 PS4_SYSV_ABI sceAvPlayerDisableStream(AvPlayerHandle handle, u32 stream_id) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerEnableStream(AvPlayerHandle handle, u32 stream_id) {
    LOG_TRACE(Lib_AvPlayer, "stream_id = {}", stream_id);
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return handle->EnableStream(stream_id);
}

bool PS4_SYSV_ABI sceAvPlayerGetAudioData(AvPlayerHandle handle, AvPlayerFrameInfo* p_info) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr || p_info == nullptr) {
        return false;
    }
    return handle->GetAudioData(*p_info);
}

s32 PS4_SYSV_ABI sceAvPlayerGetStreamInfo(AvPlayerHandle handle, u32 stream_id,
                                          AvPlayerStreamInfo* p_info) {
    LOG_TRACE(Lib_AvPlayer, "stream_id = {}", stream_id);
    if (handle == nullptr || p_info == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return handle->GetStreamInfo(stream_id, *p_info);
}

bool PS4_SYSV_ABI sceAvPlayerGetVideoData(AvPlayerHandle handle, AvPlayerFrameInfo* video_info) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr || video_info == nullptr) {
        return false;
    }
    return handle->GetVideoData(*video_info);
}

bool PS4_SYSV_ABI sceAvPlayerGetVideoDataEx(AvPlayerHandle handle,
                                            AvPlayerFrameInfoEx* video_info) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr || video_info == nullptr) {
        return false;
    }
    return handle->GetVideoData(*video_info);
}

AvPlayerHandle PS4_SYSV_ABI sceAvPlayerInit(AvPlayerInitData* data) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (data == nullptr) {
        return nullptr;
    }

    if (data->memory_replacement.allocate == nullptr ||
        data->memory_replacement.allocate_texture == nullptr ||
        data->memory_replacement.deallocate == nullptr ||
        data->memory_replacement.deallocate_texture == nullptr) {
        LOG_ERROR(Lib_AvPlayer, "All allocators are required for AvPlayer Initialisation.");
        return nullptr;
    }

    return new AvPlayer(*data);
}

s32 PS4_SYSV_ABI sceAvPlayerInitEx(const AvPlayerInitDataEx* p_data, AvPlayerHandle* p_player) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (p_data == nullptr || p_player == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }

    if (p_data->memory_replacement.allocate == nullptr ||
        p_data->memory_replacement.allocate_texture == nullptr ||
        p_data->memory_replacement.deallocate == nullptr ||
        p_data->memory_replacement.deallocate_texture == nullptr) {
        LOG_ERROR(Lib_AvPlayer, "All allocators are required for AvPlayer Initialisation.");
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }

    AvPlayerInitData data = {};
    data.memory_replacement = p_data->memory_replacement;
    data.file_replacement = p_data->file_replacement;
    data.event_replacement = p_data->event_replacement;
    data.default_language = p_data->default_language;
    data.num_output_video_framebuffers = p_data->num_output_video_framebuffers;
    data.auto_start = p_data->auto_start;

    *p_player = new AvPlayer(data);
    return ORBIS_OK;
}

bool PS4_SYSV_ABI sceAvPlayerIsActive(AvPlayerHandle handle) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr) {
        return false;
    }
    return handle->IsActive();
}

s32 PS4_SYSV_ABI sceAvPlayerJumpToTime(AvPlayerHandle handle, uint64_t time) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called, time (msec) = {}", time);
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerPause(AvPlayerHandle handle) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return handle->Pause();
}

s32 PS4_SYSV_ABI sceAvPlayerPostInit(AvPlayerHandle handle, AvPlayerPostInitData* data) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr || data == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return handle->PostInit(*data);
}

s32 PS4_SYSV_ABI sceAvPlayerPrintf(const char* format, ...) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerResume(AvPlayerHandle handle) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return handle->Resume();
}

s32 PS4_SYSV_ABI sceAvPlayerSetAvSyncMode(AvPlayerHandle handle, AvPlayerAvSyncMode sync_mode) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return handle->SetAvSyncMode(sync_mode);
}

s32 PS4_SYSV_ABI sceAvPlayerSetLogCallback(AvPlayerLogCallback log_cb, void* user_data) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerSetLooping(AvPlayerHandle handle, bool loop_flag) {
    LOG_TRACE(Lib_AvPlayer, "called, looping = {}", loop_flag);
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    if (!handle->SetLooping(loop_flag)) {
        return ORBIS_AVPLAYER_ERROR_OPERATION_FAILED;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerSetTrickSpeed(AvPlayerHandle handle, s32 trick_speed) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called speed = {}", trick_speed);
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAvPlayerStart(AvPlayerHandle handle) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return handle->Start();
}

s32 PS4_SYSV_ABI sceAvPlayerStop(AvPlayerHandle handle) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return handle->Stop();
}

s32 PS4_SYSV_ABI sceAvPlayerStreamCount(AvPlayerHandle handle) {
    LOG_TRACE(Lib_AvPlayer, "called");
    if (handle == nullptr) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }
    return handle->GetStreamCount();
}

s32 PS4_SYSV_ABI sceAvPlayerVprintf(const char* format, va_list args) {
    LOG_ERROR(Lib_AvPlayer, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("KMcEa+rHsIo", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerAddSource);
    LIB_FUNCTION("x8uvuFOPZhU", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerAddSourceEx);
    LIB_FUNCTION("buMCiJftcfw", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerChangeStream);
    LIB_FUNCTION("NkJwDzKmIlw", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerClose);
    LIB_FUNCTION("wwM99gjFf1Y", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerCurrentTime);
    LIB_FUNCTION("BOVKAzRmuTQ", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerDisableStream);
    LIB_FUNCTION("ODJK2sn9w4A", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerEnableStream);
    LIB_FUNCTION("Wnp1OVcrZgk", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerGetAudioData);
    LIB_FUNCTION("d8FcbzfAdQw", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerGetStreamInfo);
    LIB_FUNCTION("o3+RWnHViSg", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerGetVideoData);
    LIB_FUNCTION("JdksQu8pNdQ", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerGetVideoDataEx);
    LIB_FUNCTION("aS66RI0gGgo", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerInit);
    LIB_FUNCTION("o9eWRkSL+M4", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerInitEx);
    LIB_FUNCTION("UbQoYawOsfY", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerIsActive);
    LIB_FUNCTION("XC9wM+xULz8", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerJumpToTime);
    LIB_FUNCTION("9y5v+fGN4Wk", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerPause);
    LIB_FUNCTION("HD1YKVU26-M", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerPostInit);
    // LIB_FUNCTION("agig-iDRrTE", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerPrintf);
    LIB_FUNCTION("w5moABNwnRY", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerResume);
    LIB_FUNCTION("k-q+xOxdc3E", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerSetAvSyncMode);
    LIB_FUNCTION("eBTreZ84JFY", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerSetLogCallback);
    LIB_FUNCTION("OVths0xGfho", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerSetLooping);
    LIB_FUNCTION("av8Z++94rs0", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerSetTrickSpeed);
    LIB_FUNCTION("ET4Gr-Uu07s", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerStart);
    LIB_FUNCTION("ZC17w3vB5Lo", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerStop);
    LIB_FUNCTION("hdTyRzCXQeQ", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerStreamCount);
    LIB_FUNCTION("yN7Jhuv8g24", "libSceAvPlayer", 1, "libSceAvPlayer", sceAvPlayerVprintf);
};

} // namespace Libraries::AvPlayer
