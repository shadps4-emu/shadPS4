// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "libc_internal_mspace.h"

namespace Libraries::LibcInternal {

s32 PS4_SYSV_ABI sceLibcMspaceAlignedAlloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceCalloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

void* PS4_SYSV_ABI sceLibcMspaceCreate(char* name, u64 param_2, u64 param_3, u32 param_4,
                                       s8* param_5) {
    UNREACHABLE_MSG("Missing sceLibcMspace impementation!");
    return 0;
}

s32 PS4_SYSV_ABI sceLibcMspaceDestroy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceFree() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceGetAddressRanges() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceIsHeapEmpty() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceMalloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceMallocStats() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceMallocStatsFast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceMallocUsableSize() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceMemalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspacePosixMemalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceRealloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceReallocalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceSetMallocCallback() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceCalloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceCheckMemoryBounds() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceCreate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceDestroy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceFree() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceGetFooterValue() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceIsHeapEmpty() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceMalloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceMallocStats() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceMallocStatsFast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceMallocUsableSize() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceMemalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspacePosixMemalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceRealloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceReallocalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceReportMemoryBlocks() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceTrim() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceLibcInternalMspace(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("ljkqMcC4-mk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceAlignedAlloc);
    LIB_FUNCTION("LYo3GhIlB38", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceCalloc);
    LIB_FUNCTION("-hn1tcVHq5Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceCreate);
    LIB_FUNCTION("W6SiVSiCDtI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceDestroy);
    LIB_FUNCTION("Vla-Z+eXlxo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceFree);
    LIB_FUNCTION("raRgiuQfvWk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceGetAddressRanges);
    LIB_FUNCTION("pzUa7KEoydw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceIsHeapEmpty);
    LIB_FUNCTION("OJjm-QOIHlI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceMalloc);
    LIB_FUNCTION("mfHdJTIvhuo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceMallocStats);
    LIB_FUNCTION("k04jLXu3+Ic", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceMallocStatsFast);
    LIB_FUNCTION("fEoW6BJsPt4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceMallocUsableSize);
    LIB_FUNCTION("iF1iQHzxBJU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceMemalign);
    LIB_FUNCTION("qWESlyXMI3E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspacePosixMemalign);
    LIB_FUNCTION("gigoVHZvVPE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceRealloc);
    LIB_FUNCTION("p6lrRW8-MLY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceReallocalign);
    LIB_FUNCTION("+CbwGRMnlfU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceSetMallocCallback);
    LIB_FUNCTION("-lZdT34nAAE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceCalloc);
    LIB_FUNCTION("Pcq7UoYAcFE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceCheckMemoryBounds);
    LIB_FUNCTION("6hdfGRKHefs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceCreate);
    LIB_FUNCTION("qB5nGjWa-bk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceDestroy);
    LIB_FUNCTION("9mMuuhXMwqQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceFree);
    LIB_FUNCTION("kv4kgdjswN0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceGetFooterValue);
    LIB_FUNCTION("htdTOnMxDbQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceIsHeapEmpty);
    LIB_FUNCTION("QuZzFJD5Hrw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceMalloc);
    LIB_FUNCTION("mO8NB8whKy8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceMallocStats);
    LIB_FUNCTION("OmG3YPCBLJs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceMallocStatsFast);
    LIB_FUNCTION("6JcY5RDA4jY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceMallocUsableSize);
    LIB_FUNCTION("PKJcFUfhKtw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceMemalign);
    LIB_FUNCTION("7hOUKGcT6jM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspacePosixMemalign);
    LIB_FUNCTION("u32UXVridxQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceRealloc);
    LIB_FUNCTION("4SvlEtd0j40", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceReallocalign);
    LIB_FUNCTION("0FnzR6qum90", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceReportMemoryBlocks);
    LIB_FUNCTION("AUYdq63RG3U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceTrim);
}

} // namespace Libraries::LibcInternal
