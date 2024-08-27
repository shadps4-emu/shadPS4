// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ajm.h"
#include "ajm_error.h"

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::Ajm {

int PS4_SYSV_ABI sceAjmBatchCancel() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmBatchErrorDump() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmBatchJobControlBufferRa() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmBatchJobInlineBuffer() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmBatchJobRunBufferRa() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmBatchJobRunSplitBufferRa() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmBatchStartBuffer() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmBatchWait() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmDecAt9ParseConfigData() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmDecMp3ParseFrame() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmFinalize() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInitialize() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInstanceCodecType() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInstanceCreate() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInstanceDestroy() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInstanceExtend() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInstanceSwitch() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmMemoryRegister() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmMemoryUnregister() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmModuleRegister() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmModuleUnregister() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmStrError() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceAjm(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("NVDXiUesSbA", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchCancel);
    LIB_FUNCTION("WfAiBW8Wcek", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchErrorDump);
    LIB_FUNCTION("dmDybN--Fn8", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchJobControlBufferRa);
    LIB_FUNCTION("stlghnic3Jc", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchJobInlineBuffer);
    LIB_FUNCTION("ElslOCpOIns", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchJobRunBufferRa);
    LIB_FUNCTION("7jdAXK+2fMo", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchJobRunSplitBufferRa);
    LIB_FUNCTION("fFFkk0xfGWs", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchStartBuffer);
    LIB_FUNCTION("-qLsfDAywIY", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchWait);
    LIB_FUNCTION("1t3ixYNXyuc", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmDecAt9ParseConfigData);
    LIB_FUNCTION("eDFeTyi+G3Y", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmDecMp3ParseFrame);
    LIB_FUNCTION("MHur6qCsUus", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmFinalize);
    LIB_FUNCTION("dl+4eHSzUu4", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmInitialize);
    LIB_FUNCTION("diXjQNiMu-s", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmInstanceCodecType);
    LIB_FUNCTION("AxoDrINp4J8", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmInstanceCreate);
    LIB_FUNCTION("RbLbuKv8zho", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmInstanceDestroy);
    LIB_FUNCTION("YDFR0dDVGAg", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmInstanceExtend);
    LIB_FUNCTION("rgLjmfdXocI", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmInstanceSwitch);
    LIB_FUNCTION("bkRHEYG6lEM", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmMemoryRegister);
    LIB_FUNCTION("pIpGiaYkHkM", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmMemoryUnregister);
    LIB_FUNCTION("Q3dyFuwGn64", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmModuleRegister);
    LIB_FUNCTION("Wi7DtlLV+KI", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmModuleUnregister);
    LIB_FUNCTION("AxhcqVv5AYU", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmStrError);
};

} // namespace Libraries::Ajm