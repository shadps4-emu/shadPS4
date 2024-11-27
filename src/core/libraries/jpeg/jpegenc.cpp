// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "jpeg_error.h"
#include "jpegenc.h"

namespace Libraries::JpegEnc {

constexpr s32 ORBIS_JPEG_ENC_MINIMUM_MEMORY_SIZE = 0x800;

static bool IsJpegEncHandleValid(OrbisJpegEncHandle handle) {
    return handle && Common::IsAligned(reinterpret_cast<VAddr>(handle), 0x20) &&
           handle->handle == handle;
}

s32 PS4_SYSV_ABI sceJpegEncCreate(const OrbisJpegEncCreateParam* param, void* memory,
                                  const u32 memory_size, OrbisJpegEncHandle* handle) {
    if (!param || !memory || !handle) {
        LOG_ERROR(Lib_Jpeg, "Invalid address");
        return ORBIS_JPEG_ENC_ERROR_INVALID_ADDR;
    }
    if (param->size != sizeof(OrbisJpegEncCreateParam) ||
        memory_size < ORBIS_JPEG_ENC_MINIMUM_MEMORY_SIZE) {
        LOG_ERROR(Lib_Jpeg, "Invalid size");
        return ORBIS_JPEG_ENC_ERROR_INVALID_SIZE;
    }
    if (param->attr != 0) {
        LOG_ERROR(Lib_Jpeg, "Invalid attribute");
        return ORBIS_JPEG_ENC_ERROR_INVALID_ATTR;
    }

    auto* handle_internal = reinterpret_cast<OrbisJpegEncHandleInternal*>(
        Common::AlignUp(reinterpret_cast<VAddr>(memory), 0x20));
    handle_internal->handle = handle_internal;
    handle_internal->handle_size = sizeof(OrbisJpegEncHandleInternal*);
    *handle = handle_internal;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceJpegEncDelete(OrbisJpegEncHandle handle) {
    if (!IsJpegEncHandleValid(handle)) {
        LOG_ERROR(Lib_Jpeg, "Invalid handle");
        return ORBIS_JPEG_ENC_ERROR_INVALID_HANDLE;
    }

    handle->handle = nullptr;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceJpegEncEncode(OrbisJpegEncHandle handle, const OrbisJpegEncEncodeParam* param,
                                  OrbisJpegEncOutputInfo* output_info) {
    if (!IsJpegEncHandleValid(handle)) {
        LOG_ERROR(Lib_Jpeg, "Invalid handle");
        return ORBIS_JPEG_ENC_ERROR_INVALID_HANDLE;
    }
    if (!param || !param->image || !param->jpeg) {
        LOG_ERROR(Lib_Jpeg, "Invalid address");
        return ORBIS_JPEG_ENC_ERROR_INVALID_ADDR;
    }
    if (param->image_size == 0 || param->jpeg_size == 0) {
        LOG_ERROR(Lib_Jpeg, "Invalid size");
        return ORBIS_JPEG_ENC_ERROR_INVALID_SIZE;
    }

    LOG_ERROR(Lib_Jpeg, "(STUBBED) called");

    if (output_info) {
        output_info->size = param->jpeg_size;
        output_info->height = param->image_height;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceJpegEncQueryMemorySize(const OrbisJpegEncCreateParam* param) {
    if (!param) {
        LOG_ERROR(Lib_Jpeg, "Invalid address");
        return ORBIS_JPEG_ENC_ERROR_INVALID_ADDR;
    }
    if (param->size != sizeof(OrbisJpegEncCreateParam)) {
        LOG_ERROR(Lib_Jpeg, "Invalid size");
        return ORBIS_JPEG_ENC_ERROR_INVALID_SIZE;
    }
    if (param->attr != 0) {
        LOG_ERROR(Lib_Jpeg, "Invalid attribute");
        return ORBIS_JPEG_ENC_ERROR_INVALID_ATTR;
    }
    return ORBIS_JPEG_ENC_MINIMUM_MEMORY_SIZE;
}

void RegisterlibSceJpegEnc(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("K+rocojkr-I", "libSceJpegEnc", 1, "libSceJpegEnc", 1, 1, sceJpegEncCreate);
    LIB_FUNCTION("j1LyMdaM+C0", "libSceJpegEnc", 1, "libSceJpegEnc", 1, 1, sceJpegEncDelete);
    LIB_FUNCTION("QbrU0cUghEM", "libSceJpegEnc", 1, "libSceJpegEnc", 1, 1, sceJpegEncEncode);
    LIB_FUNCTION("o6ZgXfFdWXQ", "libSceJpegEnc", 1, "libSceJpegEnc", 1, 1,
                 sceJpegEncQueryMemorySize);
};

} // namespace Libraries::JpegEnc
