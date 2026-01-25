// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <png.h>
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libpng/pngenc.h"
#include "core/libraries/libs.h"

#include "pngenc_error.h"

namespace Libraries::PngEnc {

struct PngHandler {
    png_structp png_ptr;
    png_infop info_ptr;
};

struct PngWriter {
    u8* cursor;
    u8* start;
    size_t capacity;
    bool cancelWrite;
};

static inline int MapPngFilter(u16 filter) {
    if (filter == (u16)OrbisPngEncFilterType::All) {
        return PNG_ALL_FILTERS;
    }

    int f = 0;

    if (filter & (u16)OrbisPngEncFilterType::None)
        f |= PNG_FILTER_NONE;
    if (filter & (u16)OrbisPngEncFilterType::Sub)
        f |= PNG_FILTER_SUB;
    if (filter & (u16)OrbisPngEncFilterType::Up)
        f |= PNG_FILTER_UP;
    if (filter & (u16)OrbisPngEncFilterType::Average)
        f |= PNG_FILTER_AVG;
    if (filter & (u16)OrbisPngEncFilterType::Paeth)
        f |= PNG_FILTER_PAETH;

    return f;
}

void png_write_fn(png_structp png_ptr, png_bytep data, size_t length) {
    PngWriter* ctx = (PngWriter*)png_get_io_ptr(png_ptr);

    if ((size_t)(ctx->cursor - ctx->start) + length > ctx->capacity) {
        LOG_ERROR(Lib_Png, "PNG output buffer too small");
        ctx->cancelWrite = true;
        return;
    }

    memcpy(ctx->cursor, data, length);
    ctx->cursor += length;
}

void png_flush_fn(png_structp png_ptr) {}

void PngEncError(png_structp png_ptr, png_const_charp error_message) {
    LOG_ERROR(Lib_Png, "PNG error {}", error_message);
}

void PngEncWarning(png_structp png_ptr, png_const_charp error_message) {
    LOG_ERROR(Lib_Png, "PNG warning {}", error_message);
}

s32 PS4_SYSV_ABI scePngEncCreate(const OrbisPngEncCreateParam* param, void* memoryAddress,
                                 u32 memorySize, OrbisPngEncHandle* handle) {
    if (param == nullptr || param->attribute != 0) {
        LOG_ERROR(Lib_Png, "Invalid param");
        return ORBIS_PNG_ENC_ERROR_INVALID_ADDR;
    }

    if (memoryAddress == nullptr) {
        LOG_ERROR(Lib_Png, "Invalid memory address");
        return ORBIS_PNG_ENC_ERROR_INVALID_ADDR;
    }

    if (param->max_image_width - 1 > 1000000) {
        LOG_ERROR(Lib_Png, "Invalid Size, width = {}", param->max_image_width);
        return ORBIS_PNG_ENC_ERROR_INVALID_SIZE;
    }

    auto pngh = (PngHandler*)memoryAddress;

    pngh->png_ptr =
        png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, PngEncError, PngEncWarning);

    if (pngh->png_ptr == nullptr)
        return ORBIS_PNG_ENC_ERROR_FATAL;

    pngh->info_ptr = png_create_info_struct(pngh->png_ptr);
    if (pngh->info_ptr == nullptr) {
        png_destroy_write_struct(&pngh->png_ptr, nullptr);
        return false;
    }

    *handle = pngh;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePngEncDelete(OrbisPngEncHandle handle) {
    auto pngh = (PngHandler*)handle;
    png_destroy_write_struct(&pngh->png_ptr, &pngh->info_ptr);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePngEncEncode(OrbisPngEncHandle handle, const OrbisPngEncEncodeParam* param,
                                 OrbisPngEncOutputInfo* outputInfo) {
    LOG_TRACE(Lib_Png, "called png addr = {}, image addr = {}, image size = {}",
              (void*)param->png_mem_addr, (void*)param->image_mem_addr, param->image_mem_size);

    if (handle == nullptr) {
        LOG_ERROR(Lib_Png, "Invalid handle");
        return ORBIS_PNG_ENC_ERROR_INVALID_HANDLE;
    }

    if (param == nullptr) {
        LOG_ERROR(Lib_Png, "Invalid param");
        return ORBIS_PNG_ENC_ERROR_INVALID_PARAM;
    }

    if (param->image_mem_addr == nullptr || param->png_mem_addr == nullptr) {
        LOG_ERROR(Lib_Png, "Invalid input or output address");
        return ORBIS_PNG_ENC_ERROR_INVALID_ADDR;
    }

    if (param->png_mem_size == 0 || param->image_mem_size == 0 || param->image_height == 0 ||
        param->image_width == 0) {
        LOG_ERROR(Lib_Png, "Invalid Size");
        return ORBIS_PNG_ENC_ERROR_INVALID_SIZE;
    }

    auto pngh = (PngHandler*)handle;

    int png_color_type = PNG_COLOR_TYPE_RGB;

    if (param->color_space == OrbisPngEncColorSpace::RGBA) {
        png_color_type |= PNG_COLOR_MASK_ALPHA;
    }

    int png_interlace_type = PNG_INTERLACE_NONE;
    int png_compression_type = PNG_COMPRESSION_TYPE_DEFAULT;
    int png_filter_method = PNG_FILTER_TYPE_DEFAULT;

    PngWriter writer{};
    writer.cursor = param->png_mem_addr;
    writer.start = param->png_mem_addr;
    writer.capacity = param->png_mem_size;

    png_set_write_fn(pngh->png_ptr, &writer, png_write_fn, png_flush_fn);

    png_set_IHDR(pngh->png_ptr, pngh->info_ptr, param->image_width, param->image_height,
                 param->bit_depth, png_color_type, png_interlace_type, png_compression_type,
                 png_filter_method);

    if (param->pixel_format == OrbisPngEncPixelFormat::B8G8R8A8) {
        png_set_bgr(pngh->png_ptr);
    }

    png_set_compression_level(pngh->png_ptr, param->compression_level);
    png_set_filter(pngh->png_ptr, 0, MapPngFilter(param->filter_type));

    png_write_info(pngh->png_ptr, pngh->info_ptr);

    int channels = (png_color_type & PNG_COLOR_MASK_ALPHA) ? 4 : 3;
    size_t row_stride = param->image_width * channels;

    for (uint32_t y = 0; y < param->image_height; ++y) {
        png_bytep row = (png_bytep)param->image_mem_addr + y * row_stride;
        png_write_row(pngh->png_ptr, row);

        if (writer.cancelWrite) {
            LOG_ERROR(Lib_Png, "Ran out of room to write PNG");
            return ORBIS_PNG_ENC_ERROR_DATA_OVERFLOW;
        }
    }

    png_write_flush(pngh->png_ptr);

    png_write_end(pngh->png_ptr, pngh->info_ptr);

    if (outputInfo != nullptr) {
        outputInfo->data_size = writer.cursor - writer.start;
        outputInfo->processed_height = param->image_height;
    }

    return writer.cursor - writer.start;
}

s32 PS4_SYSV_ABI scePngEncQueryMemorySize(const OrbisPngEncCreateParam* param) {
    if (param == nullptr) {
        LOG_ERROR(Lib_Png, "Invalid Address");
        return ORBIS_PNG_ENC_ERROR_INVALID_ADDR;
    }

    if (param->attribute != 0 || param->max_filter_number > 5) {
        LOG_ERROR(Lib_Png, "Invalid Param, attribute = {}, max_filter_number = {}",
                  param->attribute, param->max_filter_number);
        return ORBIS_PNG_ENC_ERROR_INVALID_PARAM;
    }

    if (param->max_image_width - 1 > 1000000) {
        LOG_ERROR(Lib_Png, "Invalid Size, width = {}", param->max_image_width);
        return ORBIS_PNG_ENC_ERROR_INVALID_SIZE;
    }

    return sizeof(PngHandler);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("7aGTPfrqT9s", "libScePngEnc", 1, "libScePngEnc", scePngEncCreate);
    LIB_FUNCTION("RUrWdwTWZy8", "libScePngEnc", 1, "libScePngEnc", scePngEncDelete);
    LIB_FUNCTION("xgDjJKpcyHo", "libScePngEnc", 1, "libScePngEnc", scePngEncEncode);
    LIB_FUNCTION("9030RnBDoh4", "libScePngEnc", 1, "libScePngEnc", scePngEncQueryMemorySize);
};

} // namespace Libraries::PngEnc