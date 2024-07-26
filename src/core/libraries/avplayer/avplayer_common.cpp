// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "avplayer.h"
#include "avplayer_common.h"

#include <algorithm>   // std::equal
#include <cctype>      // std::tolower
#include <string_view> // std::string_view

namespace Libraries::AvPlayer {

using namespace Kernel;

Kernel::ScePthreadMutex CreateMutex(int type, const char* name) {
    ScePthreadMutexattr attr{};
    ScePthreadMutex mutex{};
    if (scePthreadMutexattrInit(&attr) == 0) {
        if (scePthreadMutexattrSettype(&attr, type) == 0) {
            if (scePthreadMutexInit(&mutex, &attr, name) != 0) {
                if (mutex != nullptr) {
                    scePthreadMutexDestroy(&mutex);
                }
                return nullptr;
            }
        }
    }
    if (attr != nullptr) {
        scePthreadMutexattrDestroy(&attr);
    }
    return mutex;
}

ScePthread CreateThread(Kernel::PthreadEntryFunc func, const ThreadParameters& params) {
    ScePthreadAttr attr;
    if (scePthreadAttrInit(&attr) != 0) {
        return nullptr;
    }
    if (scePthreadAttrSetinheritsched(&attr, 0) != 0) {
        scePthreadAttrDestroy(&attr);
        return nullptr;
    }

    SceKernelSchedParam param{.sched_priority = static_cast<int>(params.priority)};
    if (scePthreadAttrSetschedparam(&attr, &param) != 0) {
        scePthreadAttrDestroy(&attr);
        return nullptr;
    }
    if (scePthreadAttrSetstacksize(&attr, std::min(params.stack_size, 0x4000u)) != 0) {
        scePthreadAttrDestroy(&attr);
        return nullptr;
    }
    if (scePthreadAttrSetdetachstate(&attr, 0) != 0) {
        scePthreadAttrDestroy(&attr);
        return nullptr;
    }
    if (params.affinity > 0) {
        if (scePthreadAttrSetaffinity(&attr, params.affinity) != 0) {
            scePthreadAttrDestroy(&attr);
            return nullptr;
        }
    }

    ScePthread thread{};
    if (scePthreadCreate(&thread, &attr, func, params.p_user_data, params.thread_name) != 0) {
        scePthreadAttrDestroy(&attr);
        return nullptr;
    }

    scePthreadAttrDestroy(&attr);
    return thread;
}

static bool ichar_equals(char a, char b) {
    return std::tolower(static_cast<unsigned char>(a)) ==
           std::tolower(static_cast<unsigned char>(b));
}

static bool iequals(std::string_view l, std::string_view r) {
    return std::ranges::equal(l, r, ichar_equals);
}

SceAvPlayerSourceType GetSourceType(std::string_view path) {
    if (path.empty()) {
        return SCE_AVPLAYER_SOURCE_TYPE_UNKNOWN;
    }

    std::string_view name = path;
    if (path.find("://") != std::string_view::npos) {
        // This path is a URI. Strip HTTP parameters from it.
        // schema://server.domain/path/file.ext/and/beyond?param=value#paragraph ->
        // -> schema://server.domain/path/to/file.ext/and/beyond
        name = path.substr(0, path.find_first_of("?#"));
        if (name.empty()) {
            return SCE_AVPLAYER_SOURCE_TYPE_UNKNOWN;
        }
    }

    // schema://server.domain/path/to/file.ext/and/beyond -> .ext/and/beyond
    auto ext = name.substr(name.rfind('.'));
    if (ext.empty()) {
        return SCE_AVPLAYER_SOURCE_TYPE_UNKNOWN;
    }

    // .ext/and/beyond -> .ext
    ext = ext.substr(0, ext.find('/'));

    if (iequals(ext, ".mp4") || iequals(ext, ".m4v") || iequals(ext, ".m3d") ||
        iequals(ext, ".m4a") || iequals(ext, ".mov")) {
        return SCE_AVPLAYER_SOURCE_TYPE_FILE_MP4;
    }

    if (iequals(ext, ".m3u8")) {
        return SCE_AVPLAYER_SOURCE_TYPE_HLS;
    }

    return SCE_AVPLAYER_SOURCE_TYPE_UNKNOWN;
}

} // namespace Libraries::AvPlayer
