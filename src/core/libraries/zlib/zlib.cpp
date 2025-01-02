// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/equeue.h"
#include "core/libraries/libs.h"
#include "core/libraries/zlib/zlib_error.h"

namespace Libraries::Zlib {

int PS4_SYSV_ABI sceZlibInitialize(const void* buffer, std::size_t length) {
    LOG_ERROR(Lib_Zlib, "(STUBBED)called");

    // buffer and length passed as 0 is expected behavior, though module may use them in
    // specific ways.

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceZlibInflate(const void* source, u32 sourceLength, void* destination,
                                u32 destinationLength, u64* requestId) {
    LOG_ERROR(Lib_Zlib, "(STUBBED)called");

    if (!source || !sourceLength || !destination || !destinationLength ||
        destinationLength > 64_KB || destinationLength % 2_KB != 0)
        return ORBIS_ZLIB_ERROR_INVALID;

    *requestId = 0;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceZlibWaitForDone(u64* requestId, u32* timeout) {
    LOG_ERROR(Lib_Zlib, "(STUBBED)called");
    if (!requestId)
        return ORBIS_ZLIB_ERROR_INVALID;

    // timeout is checked by sceKernelWaitEqueue

    *requestId = 0;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceZlibGetResult(u64 requestId, u32* destinationLength, int* status) {
    LOG_ERROR(Lib_Zlib, "(STUBBED)called");

    if (!destinationLength || !status)
        return ORBIS_ZLIB_ERROR_INVALID;

    *destinationLength = 0;
    *status = 0;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceZlibFinalize() {
    LOG_ERROR(Lib_Zlib, "(STUBBED)called");
    return ORBIS_OK;
}

void RegisterlibSceZlib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("m1YErdIXCp4", "libSceZlib", 1, "libSceZlib", 1, 1, sceZlibInitialize);
    LIB_FUNCTION("6na+Sa-B83w", "libSceZlib", 1, "libSceZlib", 1, 1, sceZlibFinalize);
    LIB_FUNCTION("TLar1HULv1Q", "libSceZlib", 1, "libSceZlib", 1, 1, sceZlibInflate);
    LIB_FUNCTION("uB8VlDD4e0s", "libSceZlib", 1, "libSceZlib", 1, 1, sceZlibWaitForDone);
    LIB_FUNCTION("2eDcGHC0YaM", "libSceZlib", 1, "libSceZlib", 1, 1, sceZlibGetResult);
};

} // namespace Libraries::Zlib
