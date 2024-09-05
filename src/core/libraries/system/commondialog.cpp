// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/commondialog.h"

namespace Libraries::CommonDialog {

bool g_isInitialized = false;
bool g_isUsed = false;

int PS4_SYSV_ABI _ZN3sce16CommonDialogUtil12getSelfAppIdEv() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce16CommonDialogUtil6Client11closeModuleEv() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce16CommonDialogUtil6Client11updateStateEv() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce16CommonDialogUtil6Client15launchCmnDialogEv() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce16CommonDialogUtil6ClientD0Ev() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce16CommonDialogUtil6ClientD1Ev() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZN3sce16CommonDialogUtil6ClientD2Ev() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce16CommonDialogUtil6Client10isCloseReqEv() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce16CommonDialogUtil6Client13getFinishDataEPvm() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce16CommonDialogUtil6Client14getClientStateEv() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce16CommonDialogUtil6Client19isInitializedStatusEv() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce16CommonDialogUtil6Client8getAppIdEv() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce16CommonDialogUtil6Client8isFinishEv() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZNK3sce16CommonDialogUtil6Client9getResultEv() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI _ZTVN3sce16CommonDialogUtil6ClientE() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceCommonDialogInitialize() {
    if (g_isInitialized) {
        LOG_INFO(Lib_CommonDlg, "already initialized");
        return Error::ALREADY_SYSTEM_INITIALIZED;
    }
    LOG_DEBUG(Lib_CommonDlg, "initialized");
    g_isInitialized = true;
    return Error::OK;
}

bool PS4_SYSV_ABI sceCommonDialogIsUsed() {
    LOG_TRACE(Lib_CommonDlg, "called");
    return g_isUsed;
}

int PS4_SYSV_ABI Func_0FF577E4E8457883() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_41716C2CE379416C() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_483A427D8F6E0748() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6944B83E02727BDF() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_69F2DD23A8B4950C() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9954673DEAC170AD() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A7D4D3AB86CB7455() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_ADE4C51256B8350C() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B71349CF15FACAB0() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_CB18E00EFA946C64() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F2AEE270605622B0() {
    LOG_ERROR(Lib_CommonDlg, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceCommonDialog(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("2RdicdHhtGA", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZN3sce16CommonDialogUtil12getSelfAppIdEv);
    LIB_FUNCTION("I+tdxsCap08", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZN3sce16CommonDialogUtil6Client11closeModuleEv);
    LIB_FUNCTION("v4+gzuTkv6k", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZN3sce16CommonDialogUtil6Client11updateStateEv);
    LIB_FUNCTION("CwCzG0nnLg8", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZN3sce16CommonDialogUtil6Client15launchCmnDialogEv);
    LIB_FUNCTION("Ib1SMmbr07k", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZN3sce16CommonDialogUtil6ClientD0Ev);
    LIB_FUNCTION("6TIMpGvsrC4", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZN3sce16CommonDialogUtil6ClientD1Ev);
    LIB_FUNCTION("+UyKxWAnqIU", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZN3sce16CommonDialogUtil6ClientD2Ev);
    LIB_FUNCTION("bUCx72-9f0g", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZNK3sce16CommonDialogUtil6Client10isCloseReqEv);
    LIB_FUNCTION("xZtXq554Lbg", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZNK3sce16CommonDialogUtil6Client13getFinishDataEPvm);
    LIB_FUNCTION("C-EZ3PkhibQ", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZNK3sce16CommonDialogUtil6Client14getClientStateEv);
    LIB_FUNCTION("70niEKUAnZ0", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZNK3sce16CommonDialogUtil6Client19isInitializedStatusEv);
    LIB_FUNCTION("mdJgdwoM0Mo", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZNK3sce16CommonDialogUtil6Client8getAppIdEv);
    LIB_FUNCTION("87GekE1nowg", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZNK3sce16CommonDialogUtil6Client8isFinishEv);
    LIB_FUNCTION("6ljeTSi+fjs", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZNK3sce16CommonDialogUtil6Client9getResultEv);
    LIB_FUNCTION("W2MzrWix2mM", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 _ZTVN3sce16CommonDialogUtil6ClientE);
    LIB_FUNCTION("uoUpLGNkygk", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 sceCommonDialogInitialize);
    LIB_FUNCTION("BQ3tey0JmQM", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 sceCommonDialogIsUsed);
    LIB_FUNCTION("D-V35OhFeIM", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 Func_0FF577E4E8457883);
    LIB_FUNCTION("QXFsLON5QWw", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 Func_41716C2CE379416C);
    LIB_FUNCTION("SDpCfY9uB0g", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 Func_483A427D8F6E0748);
    LIB_FUNCTION("aUS4PgJye98", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 Func_6944B83E02727BDF);
    LIB_FUNCTION("afLdI6i0lQw", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 Func_69F2DD23A8B4950C);
    LIB_FUNCTION("mVRnPerBcK0", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 Func_9954673DEAC170AD);
    LIB_FUNCTION("p9TTq4bLdFU", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 Func_A7D4D3AB86CB7455);
    LIB_FUNCTION("reTFEla4NQw", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 Func_ADE4C51256B8350C);
    LIB_FUNCTION("txNJzxX6yrA", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 Func_B71349CF15FACAB0);
    LIB_FUNCTION("yxjgDvqUbGQ", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 Func_CB18E00EFA946C64);
    LIB_FUNCTION("8q7icGBWIrA", "libSceCommonDialog", 1, "libSceCommonDialog", 1, 1,
                 Func_F2AEE270605622B0);
};

} // namespace Libraries::CommonDialog
