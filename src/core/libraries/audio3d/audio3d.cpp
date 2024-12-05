// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio3d/audio3d.h"
#include "core/libraries/audio3d/audio3d_error.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::Audio3d {

int PS4_SYSV_ABI sceAudio3dInitialize(s64 iReserved) {
    LOG_INFO(Lib_Audio3d, "iReserved = {}", iReserved);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dTerminate() {
    // TODO: When not initialized or some ports still open, return ORBIS_AUDIO3D_ERROR_NOT_READY
    LOG_INFO(Lib_Audio3d, "called");
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceAudio3dGetDefaultOpenParameters(OrbisAudio3dOpenParameters* parameters) {
    if (parameters == nullptr) {
        LOG_ERROR(Lib_Audio3d, "Invalid OpenParameters ptr");
        return;
    }

    parameters->size_this = sizeof(OrbisAudio3dOpenParameters);
    parameters->granularity = 256;
    parameters->rate = OrbisAudio3dRate::Rate48000;
    parameters->max_objects = 512;
    parameters->queue_depth = 2;
    parameters->buffer_mode = OrbisAudio3dBufferMode::AdvanceAndPush;
    parameters->num_beds = 2;
}

int PS4_SYSV_ABI sceAudio3dPortOpen(OrbisUserServiceUserId iUserId,
                                    const OrbisAudio3dOpenParameters* pParameters,
                                    OrbisAudio3dPortId* pId) {
    LOG_INFO(Lib_Audio3d, "iUserId = {}", iUserId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dPortClose(OrbisAudio3dPortId uiPortId) {
    LOG_INFO(Lib_Audio3d, "uiPortId = {}", uiPortId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dPortSetAttribute(OrbisAudio3dPortId uiPortId,
                                            OrbisAudio3dAttributeId uiAttributeId,
                                            const void* pAttribute, size_t szAttribute) {
    LOG_INFO(Lib_Audio3d, "uiPortId = {}, uiAttributeId = {}, szAttribute = {}", uiPortId,
             uiAttributeId, szAttribute);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dPortFlush(OrbisAudio3dPortId uiPortId) {
    LOG_INFO(Lib_Audio3d, "uiPortId = {}", uiPortId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dPortAdvance(OrbisAudio3dPortId uiPortId) {
    LOG_TRACE(Lib_Audio3d, "uiPortId = {}", uiPortId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dPortPush(OrbisAudio3dPortId uiPortId, OrbisAudio3dBlocking eBlocking) {
    LOG_TRACE(Lib_Audio3d, "uiPortId = {}", uiPortId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dPortGetAttributesSupported(OrbisAudio3dPortId uiPortId,
                                                      OrbisAudio3dAttributeId* pCapabilities,
                                                      u32* pNumCapabilities) {
    LOG_INFO(Lib_Audio3d, "uiPortId = {}", uiPortId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dPortGetQueueLevel(OrbisAudio3dPortId uiPortId, u32* pQueueLevel,
                                             u32* pQueueAvailable) {
    LOG_INFO(Lib_Audio3d, "uiPortId = {}", uiPortId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dObjectReserve(OrbisAudio3dPortId uiPortId, OrbisAudio3dObjectId* pId) {
    LOG_INFO(Lib_Audio3d, "uiPortId = {}", uiPortId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dObjectUnreserve(OrbisAudio3dPortId uiPortId,
                                           OrbisAudio3dObjectId uiObjectId) {
    LOG_INFO(Lib_Audio3d, "uiPortId = {}, uiObjectId = {}", uiPortId, uiObjectId);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dObjectSetAttributes(OrbisAudio3dPortId uiPortId,
                                               OrbisAudio3dObjectId uiObjectId,
                                               size_t szNumAttributes,
                                               const OrbisAudio3dAttribute* pAttributeArray) {
    LOG_INFO(Lib_Audio3d, "uiPortId = {}, uiObjectId = {}, szNumAttributes = {}", uiPortId,
             uiObjectId, szNumAttributes);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dBedWrite(OrbisAudio3dPortId uiPortId, u32 uiNumChannels,
                                    OrbisAudio3dFormat eFormat, const void* pBuffer,
                                    u32 uiNumSamples) {
    LOG_TRACE(Lib_Audio3d, "uiPortId = {}, uiNumChannels = {}, uiNumSamples = {}", uiPortId,
              uiNumChannels, uiNumSamples);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dBedWrite2(OrbisAudio3dPortId uiPortId, u32 uiNumChannels,
                                     OrbisAudio3dFormat eFormat, const void* pBuffer,
                                     u32 uiNumSamples, OrbisAudio3dOutputRoute eOutputRoute,
                                     bool bRestricted) {
    LOG_INFO(Lib_Audio3d, "uiPortId = {}, uiNumChannels = {}, uiNumSamples = {}, bRestricted = {}",
             uiPortId, uiNumChannels, uiNumSamples, bRestricted);
    return ORBIS_OK;
}

size_t PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMemorySize(u32 uiNumSpeakers, bool bIs3d) {
    LOG_INFO(Lib_Audio3d, "uiNumSpeakers = {}, bIs3d = {}", uiNumSpeakers, bIs3d);
    return ORBIS_OK;
}

int PS4_SYSV_ABI
sceAudio3dCreateSpeakerArray(OrbisAudio3dSpeakerArrayHandle* pHandle,
                             const OrbisAudio3dSpeakerArrayParameters* pParameters) {
    if (pHandle == nullptr || pParameters == nullptr) {
        LOG_ERROR(Lib_Audio3d, "invalid SpeakerArray parameters");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }
    LOG_INFO(Lib_Audio3d, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dDeleteSpeakerArray(OrbisAudio3dSpeakerArrayHandle handle) {
    if (handle == nullptr) {
        LOG_ERROR(Lib_Audio3d, "invalid SpeakerArrayHandle");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }
    LOG_INFO(Lib_Audio3d, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMixCoefficients(OrbisAudio3dSpeakerArrayHandle handle,
                                                          OrbisAudio3dPosition pos, float fSpread,
                                                          float* pCoefficients,
                                                          u32 uiNumCoefficients) {
    LOG_INFO(Lib_Audio3d, "fSpread = {}, uiNumCoefficients = {}", fSpread, uiNumCoefficients);
    if (handle == nullptr) {
        LOG_ERROR(Lib_Audio3d, "invalid SpeakerArrayHandle");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dGetSpeakerArrayMixCoefficients2(OrbisAudio3dSpeakerArrayHandle handle,
                                                           OrbisAudio3dPosition pos, float fSpread,
                                                           float* pCoefficients,
                                                           u32 uiNumCoefficients, bool bHeightAware,
                                                           float fDownmixSpreadRadius) {
    LOG_INFO(Lib_Audio3d,
             "fSpread = {}, uiNumCoefficients = {}, bHeightAware = {}, fDownmixSpreadRadius = {}",
             fSpread, uiNumCoefficients, bHeightAware, fDownmixSpreadRadius);
    if (handle == nullptr) {
        LOG_ERROR(Lib_Audio3d, "invalid SpeakerArrayHandle");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dAudioOutOpen(OrbisAudio3dPortId uiPortId, OrbisUserServiceUserId userId,
                                        s32 type, s32 index, u32 len, u32 freq, u32 param) {
    LOG_INFO(Lib_Audio3d,
             "uiPortId = {}, userId = {}, type = {}, index = {}, len = {}, freq = {}, param = {}",
             uiPortId, userId, type, index, len, freq, param);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dAudioOutClose(s32 handle) {
    LOG_INFO(Lib_Audio3d, "handle = {}", handle);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dAudioOutOutput(s32 handle, const void* ptr) {
    LOG_TRACE(Lib_Audio3d, "handle = {}", handle);
    if (ptr == nullptr) {
        LOG_ERROR(Lib_Audio3d, "invalid Output ptr");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudio3dAudioOutOutputs(::Libraries::AudioOut::OrbisAudioOutOutputParam* param,
                                           s32 num) {
    LOG_INFO(Lib_Audio3d, "num = {}", num);
    if (param == nullptr) {
        LOG_ERROR(Lib_Audio3d, "invalid OutputParam ptr");
        return ORBIS_AUDIO3D_ERROR_INVALID_PARAMETER;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dPortCreate(u32 uiGranularity, OrbisAudio3dRate eRate, s64 iReserved,
                                      OrbisAudio3dPortId* pId) {
    LOG_INFO(Lib_Audio3d, "uiGranularity = {}, iReserved = {}", uiGranularity, iReserved);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dPortDestroy(OrbisAudio3dPortId uiPortId) {
    LOG_INFO(Lib_Audio3d, "uiPortId = {}", uiPortId);
    return ORBIS_OK;
}

// Audio3dPrivate

const char* PS4_SYSV_ABI sceAudio3dStrError(int iErrorCode) {
    LOG_ERROR(Lib_Audio3d, "(PRIVATE) called, iErrorCode = {}", iErrorCode);
    return "NOT_IMPLEMENTED";
}

// Audio3dDebug

int PS4_SYSV_ABI sceAudio3dPortQueryDebug() {
    LOG_WARNING(Lib_Audio3d, "(DEBUG) sceAudio3dPortQueryDebug called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dPortGetList() {
    LOG_WARNING(Lib_Audio3d, "(DEBUG) sceAudio3dPortGetList called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dPortGetParameters() {
    LOG_WARNING(Lib_Audio3d, "(DEBUG) sceAudio3dPortGetParameters called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dPortGetState() {
    LOG_WARNING(Lib_Audio3d, "(DEBUG) sceAudio3dPortGetState called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dPortFreeState() {
    LOG_WARNING(Lib_Audio3d, "(DEBUG) sceAudio3dPortFreeState called");
    return ORBIS_OK;
}

// Unknown

int PS4_SYSV_ABI sceAudio3dPortGetBufferLevel() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dPortGetStatus() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dReportRegisterHandler() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dReportUnregisterHandler() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAudio3dSetGpuRenderer() {
    LOG_ERROR(Lib_Audio3d, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceAudio3d(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("-R1DukFq7Dk", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dGetSpeakerArrayMixCoefficients);
    LIB_FUNCTION("-Re+pCWvwjQ", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dGetSpeakerArrayMixCoefficients2);
    LIB_FUNCTION("-pzYDZozm+M", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dPortQueryDebug);
    LIB_FUNCTION("1HXxo-+1qCw", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dObjectUnreserve);
    LIB_FUNCTION("4uyHN9q4ZeU", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dObjectSetAttributes);
    LIB_FUNCTION("7NYEzJ9SJbM", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dAudioOutOutput);
    LIB_FUNCTION("8hm6YdoQgwg", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dDeleteSpeakerArray);
    LIB_FUNCTION("9ZA23Ia46Po", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dPortGetAttributesSupported);
    LIB_FUNCTION("9tEwE0GV0qo", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dBedWrite);
    LIB_FUNCTION("Aacl5qkRU6U", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dStrError);
    LIB_FUNCTION("CKHlRW2E9dA", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dPortGetState);
    LIB_FUNCTION("HbxYY27lK6E", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dAudioOutOutputs);
    LIB_FUNCTION("Im+jOoa5WAI", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dGetDefaultOpenParameters);
    LIB_FUNCTION("Mw9mRQtWepY", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dPortDestroy);
    LIB_FUNCTION("OyVqOeVNtSk", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dPortClose);
    LIB_FUNCTION("QfNXBrKZeI0", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dReportRegisterHandler);
    LIB_FUNCTION("QqgTQQdzEMY", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dPortGetBufferLevel);
    LIB_FUNCTION("SEggctIeTcI", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dPortGetList);
    LIB_FUNCTION("UHFOgVNz0kk", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dPortCreate);
    LIB_FUNCTION("UmCvjSmuZIw", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dInitialize);
    LIB_FUNCTION("VEVhZ9qd4ZY", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dPortPush);
    LIB_FUNCTION("WW1TS2iz5yc", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dTerminate);
    LIB_FUNCTION("XeDDK0xJWQA", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dPortOpen);
    LIB_FUNCTION("YaaDbDwKpFM", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dPortGetQueueLevel);
    LIB_FUNCTION("Yq9bfUQ0uJg", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dPortSetAttribute);
    LIB_FUNCTION("ZOGrxWLgQzE", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dPortFlush);
    LIB_FUNCTION("flPcUaXVXcw", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dPortGetParameters);
    LIB_FUNCTION("iRX6GJs9tvE", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dPortGetStatus);
    LIB_FUNCTION("jO2tec4dJ2M", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dObjectReserve);
    LIB_FUNCTION("kEqqyDkmgdI", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dGetSpeakerArrayMemorySize);
    LIB_FUNCTION("lvWMW6vEqFU", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dCreateSpeakerArray);
    LIB_FUNCTION("lw0qrdSjZt8", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dPortAdvance);
    LIB_FUNCTION("pZlOm1aF3aA", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dAudioOutClose);
    LIB_FUNCTION("psv2gbihC1A", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dReportUnregisterHandler);
    LIB_FUNCTION("uJ0VhGcxCTQ", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dPortFreeState);
    LIB_FUNCTION("ucEsi62soTo", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dAudioOutOpen);
    LIB_FUNCTION("xH4Q9UILL3o", "libSceAudio3d", 1, "libSceAudio3d", 1, 1, sceAudio3dBedWrite2);
    LIB_FUNCTION("yEYXcbAGK14", "libSceAudio3d", 1, "libSceAudio3d", 1, 1,
                 sceAudio3dSetGpuRenderer);
};

} // namespace Libraries::Audio3d
