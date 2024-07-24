// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <core/file_format/playgo_chunk.h>
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "playgo.h"

namespace Libraries::PlayGo {

s32 PS4_SYSV_ABI sceDbgPlayGoRequestNextChunk() {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceDbgPlayGoSnapshot() {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoClose() {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoGetChunkId() {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoGetEta() {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoGetInstallSpeed() {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoGetLanguageMask(OrbisPlayGoHandle handle,
                                          OrbisPlayGoLanguageMask* languageMask) {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    *languageMask = 1; // En, todo;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoGetLocus(OrbisPlayGoHandle handle, const OrbisPlayGoChunkId* chunkIds,
                                   uint32_t numberOfEntries, OrbisPlayGoLocus* outLoci) {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called handle = {}, chunkIds = {}, numberOfEntries = {}",
              handle, *chunkIds, numberOfEntries);

    auto* playgo = Common::Singleton<PlaygoChunk>::Instance();

    for (uint32_t i = 0; i < numberOfEntries; i++) {
        if (chunkIds[i] <= playgo->GetPlaygoHeader().mchunk_count) {
            outLoci[i] = OrbisPlayGoLocusValue::ORBIS_PLAYGO_LOCUS_LOCAL_FAST;
        } else {
            return ORBIS_PLAYGO_ERROR_BAD_CHUNK_ID;
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoGetProgress(OrbisPlayGoHandle handle, const OrbisPlayGoChunkId* chunkIds,
                                      uint32_t numberOfEntries, OrbisPlayGoProgress* outProgress) {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called handle = {}, chunkIds = {}, numberOfEntries = {}",
              handle, *chunkIds, numberOfEntries);
    outProgress->progressSize = 0x10000; // todo?
    outProgress->totalSize = 0x10000;
    return 0;
}

s32 PS4_SYSV_ABI scePlayGoGetToDoList(OrbisPlayGoHandle handle, OrbisPlayGoToDo* outTodoList,
                                      u32 numberOfEntries, u32* outEntries) {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    if (handle != 1)
        return ORBIS_PLAYGO_ERROR_BAD_HANDLE;
    if (outTodoList == nullptr)
        return ORBIS_PLAYGO_ERROR_BAD_POINTER;
    *outEntries = 0; // nothing to do
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoInitialize(OrbisPlayGoInitParams* param) {
    if (param->bufAddr == nullptr)
        return ORBIS_PLAYGO_ERROR_BAD_POINTER;
    if (param->bufSize < 0x200000)
        return ORBIS_PLAYGO_ERROR_BAD_SIZE;
    LOG_INFO(Lib_PlayGo, "(STUBBED)called, bufSize = {}", param->bufSize);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoOpen(OrbisPlayGoHandle* outHandle, const void* param) {
    *outHandle = 1;
    LOG_INFO(Lib_PlayGo, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoPrefetch() {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoSetInstallSpeed() {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoSetLanguageMask(OrbisPlayGoHandle handle,
                                          OrbisPlayGoLanguageMask languageMask) {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoSetToDoList(OrbisPlayGoHandle handle, const OrbisPlayGoToDo* todoList,
                                      uint32_t numberOfEntries) {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePlayGoTerminate() {
    LOG_ERROR(Lib_PlayGo, "(STUBBED)called");
    return ORBIS_OK;
}

void RegisterlibScePlayGo(Core::Loader::SymbolsResolver* sym) {
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
