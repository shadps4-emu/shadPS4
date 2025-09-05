// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_format/playgo_chunk.h"
#include "core/file_sys/fs.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/systemservice.h"
#include "playgo.h"

namespace Libraries::PlayGo {

static constexpr OrbisPlayGoHandle PlaygoHandle = 1;
static std::unique_ptr<PlaygoFile> playgo;

s32 PS4_SYSV_ABI sceDbgPlayGoRequestNextChunk() {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceDbgPlayGoSnapshot() {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoClose(OrbisPlayGoHandle handle) {
    LOG_INFO(Lib_PlayGo, "called");

    if (handle != PlaygoHandle) {
        return ORBIS_PLAYGO_ERROR_BAD_HANDLE;
    }
    if (!playgo) {
        return ORBIS_PLAYGO_ERROR_NOT_INITIALIZED;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoGetChunkId(OrbisPlayGoHandle handle, OrbisPlayGoChunkId* outChunkIdList,
                                     u32 numberOfEntries, u32* outEntries) {
    LOG_DEBUG(Lib_PlayGo, "called");

    if (handle != PlaygoHandle) {
        return ORBIS_PLAYGO_ERROR_BAD_HANDLE;
    }
    if (outEntries == nullptr) {
        return ORBIS_PLAYGO_ERROR_BAD_POINTER;
    }
    if (outChunkIdList != nullptr && numberOfEntries == 0) {
        return ORBIS_PLAYGO_ERROR_BAD_SIZE;
    }

    if (playgo->GetPlaygoHeader().file_size == 0) {
        *outEntries = 0;
        return ORBIS_OK;
    }

    if (outChunkIdList == nullptr) {
        *outEntries = playgo->chunks.size();
        return ORBIS_OK;
    }

    if (numberOfEntries > playgo->chunks.size()) {
        numberOfEntries = playgo->chunks.size();
    }

    for (u32 i = 0; i < numberOfEntries; i++) {
        outChunkIdList[i] = i;
    }
    *outEntries = numberOfEntries;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoGetEta(OrbisPlayGoHandle handle, const OrbisPlayGoChunkId* chunkIds,
                                 u32 numberOfEntries, OrbisPlayGoEta* outEta) {
    LOG_DEBUG(Lib_PlayGo, "called");
    if (handle != PlaygoHandle) {
        return ORBIS_PLAYGO_ERROR_BAD_HANDLE;
    }
    if (chunkIds == nullptr || outEta == nullptr) {
        return ORBIS_PLAYGO_ERROR_BAD_POINTER;
    }
    if (numberOfEntries == 0) {
        return ORBIS_PLAYGO_ERROR_BAD_SIZE;
    }

    *outEta = 0; // all is loaded
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoGetInstallSpeed(OrbisPlayGoHandle handle,
                                          OrbisPlayGoInstallSpeed* outSpeed) {
    LOG_INFO(Lib_PlayGo, "called");

    if (handle != PlaygoHandle) {
        return ORBIS_PLAYGO_ERROR_BAD_HANDLE;
    }
    if (outSpeed == nullptr) {
        return ORBIS_PLAYGO_ERROR_BAD_POINTER;
    }
    if (!playgo) {
        return ORBIS_PLAYGO_ERROR_NOT_INITIALIZED;
    }

    std::scoped_lock lk{playgo->GetSpeedMutex()};

    if (playgo->speed == OrbisPlayGoInstallSpeed::Suspended) {
        using namespace std::chrono;
        if ((duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count() -
             playgo->speed_tick) > 30 * 1000) { // 30sec
            playgo->speed = OrbisPlayGoInstallSpeed::Trickle;
        }
    }
    *outSpeed = playgo->speed;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoGetLanguageMask(OrbisPlayGoHandle handle,
                                          OrbisPlayGoLanguageMask* outLanguageMask) {
    LOG_INFO(Lib_PlayGo, "called");

    if (handle != PlaygoHandle) {
        return ORBIS_PLAYGO_ERROR_BAD_HANDLE;
    }
    if (outLanguageMask == nullptr) {
        return ORBIS_PLAYGO_ERROR_BAD_POINTER;
    }
    if (!playgo) {
        return ORBIS_PLAYGO_ERROR_NOT_INITIALIZED;
    }

    *outLanguageMask = playgo->langMask;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoGetLocus(OrbisPlayGoHandle handle, const OrbisPlayGoChunkId* chunkIds,
                                   uint32_t numberOfEntries, OrbisPlayGoLocus* outLoci) {
    if (handle != PlaygoHandle) {
        return ORBIS_PLAYGO_ERROR_BAD_HANDLE;
    }
    if (chunkIds == nullptr || outLoci == nullptr) {
        return ORBIS_PLAYGO_ERROR_BAD_POINTER;
    }
    if (numberOfEntries == 0) {
        return ORBIS_PLAYGO_ERROR_BAD_SIZE;
    }

    LOG_DEBUG(Lib_PlayGo, "called handle = {}, chunkIds = {}, numberOfEntries = {}", handle,
              *chunkIds, numberOfEntries);

    if (!playgo) {
        return ORBIS_PLAYGO_ERROR_NOT_INITIALIZED;
    }
    if (playgo->GetPlaygoHeader().file_size == 0) {
        return ORBIS_PLAYGO_ERROR_NOT_SUPPORT_PLAYGO;
    }

    for (int i = 0; i < numberOfEntries; i++) {
        if (chunkIds[i] < playgo->chunks.size()) {
            outLoci[i] = OrbisPlayGoLocus::LocalFast;
        } else {
            outLoci[i] = OrbisPlayGoLocus::NotDownloaded;
            return ORBIS_PLAYGO_ERROR_BAD_CHUNK_ID;
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoGetProgress(OrbisPlayGoHandle handle, const OrbisPlayGoChunkId* chunkIds,
                                      uint32_t numberOfEntries, OrbisPlayGoProgress* outProgress) {
    LOG_DEBUG(Lib_PlayGo, "called handle = {}, chunkIds = {}, numberOfEntries = {}", handle,
              *chunkIds, numberOfEntries);

    if (handle != PlaygoHandle) {
        return ORBIS_PLAYGO_ERROR_BAD_HANDLE;
    }
    if (chunkIds == nullptr || outProgress == nullptr) {
        return ORBIS_PLAYGO_ERROR_BAD_POINTER;
    }
    if (numberOfEntries == 0) {
        return ORBIS_PLAYGO_ERROR_BAD_SIZE;
    }
    if (!playgo) {
        return ORBIS_PLAYGO_ERROR_NOT_INITIALIZED;
    }
    if (playgo->GetPlaygoHeader().file_size == 0) {
        return ORBIS_PLAYGO_ERROR_BAD_CHUNK_ID;
    }

    outProgress->progressSize = 0;
    outProgress->totalSize = 0;

    u64 total_size = 0;
    for (u32 i = 0; i < numberOfEntries; i++) {
        u32 chunk_id = chunkIds[i];
        if (chunk_id < playgo->chunks.size()) {
            total_size += playgo->chunks[chunk_id].total_size;
        } else {
            return ORBIS_PLAYGO_ERROR_BAD_CHUNK_ID;
        }
    }

    outProgress->progressSize = total_size;
    outProgress->totalSize = total_size;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoGetToDoList(OrbisPlayGoHandle handle, OrbisPlayGoToDo* outTodoList,
                                      u32 numberOfEntries, u32* outEntries) {
    LOG_INFO(Lib_PlayGo, "called handle = {} numberOfEntries = {}", handle, numberOfEntries);

    if (handle != PlaygoHandle) {
        return ORBIS_PLAYGO_ERROR_BAD_HANDLE;
    }
    if (outTodoList == nullptr || outEntries == nullptr) {
        return ORBIS_PLAYGO_ERROR_BAD_POINTER;
    }
    if (numberOfEntries == 0) {
        return ORBIS_PLAYGO_ERROR_BAD_SIZE;
    }
    if (!playgo) {
        return ORBIS_PLAYGO_ERROR_NOT_INITIALIZED;
    }
    *outEntries = 0; // nothing to do
    return ORBIS_OK;
}

int scePlayGoConvertLanguage(int systemLang) {
    if (systemLang >= 0 && systemLang < 48) {
        return (1 << (64 - systemLang - 1));
    } else {
        return 0;
    }
}

s32 PS4_SYSV_ABI scePlayGoInitialize(OrbisPlayGoInitParams* param) {
    LOG_INFO(Lib_PlayGo, "called, bufSize = {}", param->bufSize);
    if (param->bufAddr == nullptr) {
        return ORBIS_PLAYGO_ERROR_BAD_POINTER;
    }
    if (param->bufSize < 0x200000) {
        return ORBIS_PLAYGO_ERROR_BAD_SIZE;
    }
    if (playgo) {
        return ORBIS_PLAYGO_ERROR_ALREADY_INITIALIZED;
    }

    using namespace SystemService;

    playgo = std::make_unique<PlaygoFile>();

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    const auto file_path = mnt->GetHostPath("/app0/sce_sys/playgo-chunk.dat");
    if (!playgo->Open(file_path)) {
        LOG_WARNING(Lib_PlayGo, "Could not open PlayGo file");
    }

    s32 system_lang = 0;
    sceSystemServiceParamGetInt(OrbisSystemServiceParamId::Lang, &system_lang);
    playgo->langMask = scePlayGoConvertLanguage(system_lang);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoOpen(OrbisPlayGoHandle* outHandle, const void* param) {
    LOG_INFO(Lib_PlayGo, "called");

    if (outHandle == nullptr) {
        return ORBIS_PLAYGO_ERROR_BAD_POINTER;
    }
    if (param) {
        return ORBIS_PLAYGO_ERROR_INVALID_ARGUMENT;
    }
    if (!playgo) {
        return ORBIS_PLAYGO_ERROR_NOT_INITIALIZED;
    }
    if (playgo->GetPlaygoHeader().file_size == 0) {
        return ORBIS_PLAYGO_ERROR_NOT_SUPPORT_PLAYGO;
    }

    playgo->handle = *outHandle = PlaygoHandle;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoPrefetch(OrbisPlayGoHandle handle, const OrbisPlayGoChunkId* chunkIds,
                                   u32 numberOfEntries, OrbisPlayGoLocus minimumLocus) {
    LOG_INFO(Lib_PlayGo, "called");

    if (handle != PlaygoHandle) {
        return ORBIS_PLAYGO_ERROR_BAD_HANDLE;
    }
    if (chunkIds == nullptr) {
        return ORBIS_PLAYGO_ERROR_BAD_POINTER;
    }
    if (numberOfEntries == 0) {
        return ORBIS_PLAYGO_ERROR_BAD_SIZE;
    }
    if (!playgo) {
        return ORBIS_PLAYGO_ERROR_NOT_INITIALIZED;
    }

    switch (minimumLocus) {
    case OrbisPlayGoLocus::NotDownloaded:
    case OrbisPlayGoLocus::LocalSlow:
    case OrbisPlayGoLocus::LocalFast:
        break;
    default:
        return ORBIS_PLAYGO_ERROR_BAD_LOCUS;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoSetInstallSpeed(OrbisPlayGoHandle handle, OrbisPlayGoInstallSpeed speed) {
    LOG_INFO(Lib_PlayGo, "called");

    if (handle != PlaygoHandle) {
        return ORBIS_PLAYGO_ERROR_BAD_HANDLE;
    }
    if (!playgo) {
        return ORBIS_PLAYGO_ERROR_NOT_INITIALIZED;
    }

    switch (speed) {
    case OrbisPlayGoInstallSpeed::Suspended:
    case OrbisPlayGoInstallSpeed::Trickle:
    case OrbisPlayGoInstallSpeed::Full:
        break;
    default:
        return ORBIS_PLAYGO_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{playgo->GetSpeedMutex()};
    using namespace std::chrono;
    playgo->speed = speed;
    playgo->speed_tick =
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoSetLanguageMask(OrbisPlayGoHandle handle,
                                          OrbisPlayGoLanguageMask languageMask) {
    LOG_INFO(Lib_PlayGo, "called");

    if (handle != 1) {
        return ORBIS_PLAYGO_ERROR_BAD_HANDLE;
    }
    if (!playgo) {
        return ORBIS_PLAYGO_ERROR_NOT_INITIALIZED;
    }

    playgo->langMask = languageMask;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoSetToDoList(OrbisPlayGoHandle handle, const OrbisPlayGoToDo* todoList,
                                      uint32_t numberOfEntries) {
    LOG_INFO(Lib_PlayGo, "called");

    if (handle != PlaygoHandle) {
        return ORBIS_PLAYGO_ERROR_BAD_HANDLE;
    }
    if (todoList == nullptr) {
        return ORBIS_PLAYGO_ERROR_BAD_POINTER;
    }
    if (numberOfEntries == 0) {
        return ORBIS_PLAYGO_ERROR_BAD_SIZE;
    }
    if (!playgo) {
        return ORBIS_PLAYGO_ERROR_NOT_INITIALIZED;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoTerminate() {
    LOG_INFO(Lib_PlayGo, "called");

    if (!playgo) {
        return ORBIS_PLAYGO_ERROR_NOT_INITIALIZED;
    }
    playgo.reset();
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("uEqMfMITvEI", "libSceDbgPlayGo", 1, "libScePlayGo", 1, 0,
                 sceDbgPlayGoRequestNextChunk);
    LIB_FUNCTION("vU+FqrH+pEY", "libSceDbgPlayGo", 1, "libScePlayGo", 1, 0, sceDbgPlayGoSnapshot);
    LIB_FUNCTION("Uco1I0dlDi8", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoClose);
    LIB_FUNCTION("73fF1MFU8hA", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoGetChunkId);
    LIB_FUNCTION("v6EZ-YWRdMs", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoGetEta);
    LIB_FUNCTION("rvBSfTimejE", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoGetInstallSpeed);
    LIB_FUNCTION("3OMbYZBaa50", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoGetLanguageMask);
    LIB_FUNCTION("uWIYLFkkwqk", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoGetLocus);
    LIB_FUNCTION("-RJWNMK3fC8", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoGetProgress);
    LIB_FUNCTION("Nn7zKwnA5q0", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoGetToDoList);
    LIB_FUNCTION("ts6GlZOKRrE", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoInitialize);
    LIB_FUNCTION("M1Gma1ocrGE", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoOpen);
    LIB_FUNCTION("-Q1-u1a7p0g", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoPrefetch);
    LIB_FUNCTION("4AAcTU9R3XM", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoSetInstallSpeed);
    LIB_FUNCTION("LosLlHOpNqQ", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoSetLanguageMask);
    LIB_FUNCTION("gUPGiOQ1tmQ", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoSetToDoList);
    LIB_FUNCTION("MPe0EeBGM-E", "libScePlayGo", 1, "libScePlayGo", 1, 0, scePlayGoTerminate);
};

} // namespace Libraries::PlayGo
