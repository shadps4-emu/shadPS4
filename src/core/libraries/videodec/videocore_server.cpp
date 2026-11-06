// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include <unordered_map>
#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/videodec/videocore_server.h"
#include "core/libraries/videodec/videodec_error.h"

namespace Libraries::VideoCoreServer {

struct VideoCoreResource {
    u32 id;
    bool is_be_wrapper;
    OrbisVideoCoreAcquireDecoderResourceParam param;
    OrbisVideoCoreCallback callback;
};

class VideoCoreResourceManager {
public:
    static VideoCoreResourceManager& Instance() {
        static VideoCoreResourceManager instance;
        return instance;
    }

    s32 AcquireResource(const OrbisVideoCoreAcquireDecoderResourceParam* param,
                        OrbisVideoCoreCallback cb, u32* out_resource_id, bool is_be_wrapper) {
        if (!param || !cb || !out_resource_id) {
            LOG_ERROR(Lib_Videodec, "Invalid arguments passed to VideoCore acquire");
            return ORBIS_VIDEO_CORE_ERROR_INVALID_ARGUMENT;
        }

        std::unique_lock lock(mutex);
        const u32 id = next_id++;
        resources[id] = VideoCoreResource{
            .id = id,
            .is_be_wrapper = is_be_wrapper,
            .param = *param,
            .callback = cb,
        };
        *out_resource_id = id;

        LOG_INFO(Lib_Videodec,
                 "Acquired VideoCore {} decoder resource id={}, res_type={}, dim={}x{}, spec={}",
                 is_be_wrapper ? "BeWrapper" : "Standard", id, param->resource_type,
                 param->max_width, param->max_height, param->stream_spec);

        return ORBIS_OK;
    }

    s32 ReleaseResource(u32 resource_id, bool is_be_wrapper) {
        LOG_INFO(Lib_Videodec, "Releasing VideoCore {} resource_id={}",
                 is_be_wrapper ? "BeWrapper" : "Standard", resource_id);

        if (resource_id == 0) {
            return is_be_wrapper ? ORBIS_VIDEO_CORE_ERROR_BE_WRAPPER_RESOURCE_NOT_FOUND
                                 : ORBIS_VIDEO_CORE_ERROR_RESOURCE_NOT_FOUND;
        }

        std::unique_lock lock(mutex);
        auto it = resources.find(resource_id);
        if (it == resources.end()) {
            LOG_ERROR(Lib_Videodec, "Resource id={} not found", resource_id);
            return is_be_wrapper ? ORBIS_VIDEO_CORE_ERROR_BE_WRAPPER_RESOURCE_NOT_FOUND
                                 : ORBIS_VIDEO_CORE_ERROR_RESOURCE_NOT_FOUND;
        }

        if (it->second.is_be_wrapper != is_be_wrapper) {
            LOG_ERROR(Lib_Videodec,
                      "Resource id={} type mismatch: requested is_be_wrapper={}, resource "
                      "is_be_wrapper={}",
                      resource_id, is_be_wrapper, it->second.is_be_wrapper);
            return is_be_wrapper ? ORBIS_VIDEO_CORE_ERROR_BE_WRAPPER_RESOURCE_NOT_FOUND
                                 : ORBIS_VIDEO_CORE_ERROR_RESOURCE_NOT_FOUND;
        }

        resources.erase(it);
        LOG_INFO(Lib_Videodec, "Released VideoCore resource id={}", resource_id);
        return ORBIS_OK;
    }

private:
    std::mutex mutex;
    u32 next_id{1};
    std::unordered_map<u32, VideoCoreResource> resources;
};

s32 PS4_SYSV_ABI
sceVideoCoreAcquireDecoderResource(const OrbisVideoCoreAcquireDecoderResourceParam* param,
                                   OrbisVideoCoreCallback cb, u32* out_resource_id) {
    return VideoCoreResourceManager::Instance().AcquireResource(param, cb, out_resource_id, false);
}

s32 PS4_SYSV_ABI sceVideoCoreReleaseDecoderResource(u32 resource_id) {
    return VideoCoreResourceManager::Instance().ReleaseResource(resource_id, false);
}

s32 PS4_SYSV_ABI sceVideoCoreAcquireDecoderResourceForBeWrapper(
    const OrbisVideoCoreAcquireDecoderResourceParam* param, OrbisVideoCoreCallback cb,
    u32* out_resource_id) {
    return VideoCoreResourceManager::Instance().AcquireResource(param, cb, out_resource_id, true);
}

s32 PS4_SYSV_ABI sceVideoCoreReleaseDecoderResourceForBeWrapper(u32 resource_id) {
    return VideoCoreResourceManager::Instance().ReleaseResource(resource_id, true);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("vDenqj6dzGk", "libSceVideoCoreServerInterface", 1,
                 "libSceVideoCoreServerInterface", sceVideoCoreAcquireDecoderResourceForBeWrapper);
    LIB_FUNCTION("dlG8cpQGq64", "libSceVideoCoreServerInterface", 1,
                 "libSceVideoCoreServerInterface", sceVideoCoreReleaseDecoderResourceForBeWrapper);
    LIB_FUNCTION("9eP06So7cfY", "libSceVideoCoreServerInterface", 1,
                 "libSceVideoCoreServerInterface", sceVideoCoreAcquireDecoderResource);
    LIB_FUNCTION("k2ZyhP7QdaA", "libSceVideoCoreServerInterface", 1,
                 "libSceVideoCoreServerInterface", sceVideoCoreReleaseDecoderResource);
}

} // namespace Libraries::VideoCoreServer
