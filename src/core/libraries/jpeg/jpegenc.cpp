// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "jpeg_error.h"
#include "jpegenc.h"

namespace Libraries::JpegEnc {

constexpr s32 ORBIS_JPEG_ENC_MINIMUM_MEMORY_SIZE = 0x800;
constexpr u32 ORBIS_JPEG_ENC_MAX_IMAGE_DIMENSION = 0xFFFF;
constexpr u32 ORBIS_JPEG_ENC_MAX_IMAGE_PITCH = 0xFFFFFFF;
constexpr u32 ORBIS_JPEG_ENC_MAX_IMAGE_SIZE = 0x7FFFFFFF;

static s32 ValidateJpegEncCreateParam(const OrbisJpegEncCreateParam* param) {
    if (!param) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_ADDR;
    }
    if (param->size != sizeof(OrbisJpegEncCreateParam)) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_SIZE;
    }
    if (param->attr != ORBIS_JPEG_ENC_ATTRIBUTE_NONE) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_PARAM;
    }
    return ORBIS_OK;
}

static s32 ValidateJpegEncMemory(const void* memory, const u32 memory_size) {
    if (!memory) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_ADDR;
    }
    if (memory_size < ORBIS_JPEG_ENC_MINIMUM_MEMORY_SIZE) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_SIZE;
    }
    return ORBIS_OK;
}

static s32 ValidateJpegEncEncodeParam(const OrbisJpegEncEncodeParam* param) {

    // Validate addresses
    if (!param) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_ADDR;
    }
    if (!param->image || (param->pixel_format != ORBIS_JPEG_ENC_PIXEL_FORMAT_Y8 &&
                          !Common::IsAligned(reinterpret_cast<VAddr>(param->image), 4))) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_ADDR;
    }
    if (!param->jpeg) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_ADDR;
    }

    // Validate sizes
    if (param->image_size == 0 || param->jpeg_size == 0) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_SIZE;
    }

    // Validate parameters
    if (param->image_width > ORBIS_JPEG_ENC_MAX_IMAGE_DIMENSION ||
        param->image_height > ORBIS_JPEG_ENC_MAX_IMAGE_DIMENSION) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_PARAM;
    }
    if (param->image_pitch == 0 || param->image_pitch > ORBIS_JPEG_ENC_MAX_IMAGE_PITCH ||
        (param->pixel_format != ORBIS_JPEG_ENC_PIXEL_FORMAT_Y8 &&
         !Common::IsAligned(param->image_pitch, 4))) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_PARAM;
    }
    const auto calculated_size = param->image_height * param->image_pitch;
    if (calculated_size > ORBIS_JPEG_ENC_MAX_IMAGE_SIZE || calculated_size > param->image_size) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_PARAM;
    }
    if (param->encode_mode != ORBIS_JPEG_ENC_ENCODE_MODE_NORMAL &&
        param->encode_mode != ORBIS_JPEG_ENC_ENCODE_MODE_MJPEG) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_PARAM;
    }
    if (param->color_space != ORBIS_JPEG_ENC_COLOR_SPACE_YCC &&
        param->color_space != ORBIS_JPEG_ENC_COLOR_SPACE_GRAYSCALE) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_PARAM;
    }
    if (param->sampling_type != ORBIS_JPEG_ENC_SAMPLING_TYPE_FULL &&
        param->sampling_type != ORBIS_JPEG_ENC_SAMPLING_TYPE_422 &&
        param->sampling_type != ORBIS_JPEG_ENC_SAMPLING_TYPE_420) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_PARAM;
    }
    if (param->restart_interval > ORBIS_JPEG_ENC_MAX_IMAGE_DIMENSION) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_PARAM;
    }
    switch (param->pixel_format) {
    case ORBIS_JPEG_ENC_PIXEL_FORMAT_R8G8B8A8:
    case ORBIS_JPEG_ENC_PIXEL_FORMAT_B8G8R8A8:
        if (param->image_pitch >> 2 < param->image_width ||
            param->color_space != ORBIS_JPEG_ENC_COLOR_SPACE_YCC ||
            param->sampling_type == ORBIS_JPEG_ENC_SAMPLING_TYPE_FULL) {
            return ORBIS_JPEG_ENC_ERROR_INVALID_PARAM;
        }
        break;
    case ORBIS_JPEG_ENC_PIXEL_FORMAT_Y8U8Y8V8:
        if (param->image_pitch >> 1 < Common::AlignUp(param->image_width, 2) ||
            param->color_space != ORBIS_JPEG_ENC_COLOR_SPACE_YCC ||
            param->sampling_type == ORBIS_JPEG_ENC_SAMPLING_TYPE_FULL) {
            return ORBIS_JPEG_ENC_ERROR_INVALID_PARAM;
        }
        break;
    case ORBIS_JPEG_ENC_PIXEL_FORMAT_Y8:
        if (param->image_pitch < param->image_width ||
            param->color_space != ORBIS_JPEG_ENC_COLOR_SPACE_GRAYSCALE ||
            param->sampling_type != ORBIS_JPEG_ENC_SAMPLING_TYPE_FULL) {
            return ORBIS_JPEG_ENC_ERROR_INVALID_PARAM;
        }
        break;
    default:
        return ORBIS_JPEG_ENC_ERROR_INVALID_PARAM;
    }

    return ORBIS_OK;
}

static s32 ValidateJpecEngHandle(OrbisJpegEncHandle handle) {
    if (!handle || !Common::IsAligned(reinterpret_cast<VAddr>(handle), 0x20) ||
        handle->handle != handle) {
        return ORBIS_JPEG_ENC_ERROR_INVALID_HANDLE;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceJpegEncCreate(const OrbisJpegEncCreateParam* param, void* memory,
                                  const u32 memory_size, OrbisJpegEncHandle* handle) {
    if (auto param_ret = ValidateJpegEncCreateParam(param); param_ret != ORBIS_OK) {
        LOG_ERROR(Lib_Jpeg, "Invalid create param");
        return param_ret;
    }
    if (auto memory_ret = ValidateJpegEncMemory(memory, memory_size); memory_ret != ORBIS_OK) {
        LOG_ERROR(Lib_Jpeg, "Invalid memory");
        return memory_ret;
    }
    if (!handle) {
        LOG_ERROR(Lib_Jpeg, "Invalid handle output");
        return ORBIS_JPEG_ENC_ERROR_INVALID_ADDR;
    }

    auto* handle_internal = reinterpret_cast<OrbisJpegEncHandleInternal*>(
        Common::AlignUp(reinterpret_cast<VAddr>(memory), 0x20));
    handle_internal->handle = handle_internal;
    handle_internal->handle_size = sizeof(OrbisJpegEncHandleInternal*);
    *handle = handle_internal;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceJpegEncDelete(OrbisJpegEncHandle handle) {
    if (auto handle_ret = ValidateJpecEngHandle(handle); handle_ret != ORBIS_OK) {
        LOG_ERROR(Lib_Jpeg, "Invalid handle");
        return handle_ret;
    }
    handle->handle = nullptr;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceJpegEncEncode(OrbisJpegEncHandle handle, const OrbisJpegEncEncodeParam* param,
                                  OrbisJpegEncOutputInfo* output_info) {
    if (auto handle_ret = ValidateJpecEngHandle(handle); handle_ret != ORBIS_OK) {
        LOG_ERROR(Lib_Jpeg, "Invalid handle");
        return handle_ret;
    }
    if (auto param_ret = ValidateJpegEncEncodeParam(param); param_ret != ORBIS_OK) {
        LOG_ERROR(Lib_Jpeg, "Invalid encode param");
        return param_ret;
    }

    LOG_ERROR(Lib_Jpeg,
              "(STUBBED) image_size = {} , jpeg_size = {} , image_width = {} , image_height = {} , "
              "image_pitch = {} , pixel_format = {} , encode_mode = {} , color_space = {} , "
              "sampling_type = {} , compression_ratio = {} , restart_interval = {}",
              param->image_size, param->jpeg_size, param->image_width, param->image_height,
              param->image_pitch, magic_enum::enum_name(param->pixel_format),
              magic_enum::enum_name(param->encode_mode), magic_enum::enum_name(param->color_space),
              magic_enum::enum_name(param->sampling_type), param->compression_ratio,
              param->restart_interval);

    if (output_info) {
        output_info->size = param->jpeg_size;
        output_info->height = param->image_height;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceJpegEncQueryMemorySize(const OrbisJpegEncCreateParam* param) {
    if (auto param_ret = ValidateJpegEncCreateParam(param); param_ret != ORBIS_OK) {
        LOG_ERROR(Lib_Jpeg, "Invalid create param");
        return param_ret;
    }
    return ORBIS_JPEG_ENC_MINIMUM_MEMORY_SIZE;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("K+rocojkr-I", "libSceJpegEnc", 1, "libSceJpegEnc", 1, 1, sceJpegEncCreate);
    LIB_FUNCTION("j1LyMdaM+C0", "libSceJpegEnc", 1, "libSceJpegEnc", 1, 1, sceJpegEncDelete);
    LIB_FUNCTION("QbrU0cUghEM", "libSceJpegEnc", 1, "libSceJpegEnc", 1, 1, sceJpegEncEncode);
    LIB_FUNCTION("o6ZgXfFdWXQ", "libSceJpegEnc", 1, "libSceJpegEnc", 1, 1,
                 sceJpegEncQueryMemorySize);
};

} // namespace Libraries::JpegEnc
