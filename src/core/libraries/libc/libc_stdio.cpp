// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/libraries/libc/libc_stdio.h"

namespace Libraries::LibC {

std::FILE* PS4_SYSV_ABI ps4_fopen(const char* filename, const char* mode) {
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    const auto host_path = mnt->GetHostPath(filename).string();
    FILE* f = std::fopen(host_path.c_str(), mode);
    if (f != nullptr) {
        LOG_INFO(Lib_LibC, "fopen = {}", host_path);
    } else {
        LOG_INFO(Lib_LibC, "fopen can't open = {}", host_path);
    }
    return f;
}

int PS4_SYSV_ABI ps4_fclose(FILE* stream) {
    LOG_INFO(Lib_LibC, "callled");
    int ret = 0;
    if (stream != nullptr) {
        ret = fclose(stream);
    }
    return ret;
}

int PS4_SYSV_ABI ps4_setvbuf(FILE* stream, char* buf, int mode, size_t size) {
    return setvbuf(stream, buf, mode, size);
}

int PS4_SYSV_ABI ps4_fseek(FILE* stream, long offset, int whence) {
    return fseek(stream, offset, whence);
}

int PS4_SYSV_ABI ps4_fgetpos(FILE* stream, fpos_t* pos) {
    return fgetpos(stream, pos);
}

std::size_t PS4_SYSV_ABI ps4_fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fread(ptr, size, nmemb, stream);
}

int PS4_SYSV_ABI ps4_printf(VA_ARGS) {
    VA_CTX(ctx);
    return printf_ctx(&ctx);
}

int PS4_SYSV_ABI ps4_fprintf(FILE* file, VA_ARGS) {
    int fd = fileno(file);
    if (fd == 1 || fd == 2) { // output stdout and stderr to console
        VA_CTX(ctx);
        return printf_ctx(&ctx);
    } else {
        VA_CTX(ctx);
        char buf[256];
        fprintf_ctx(&ctx, buf);
        return fprintf(file, "%s", buf);
    }
}

int PS4_SYSV_ABI ps4_vsnprintf(char* s, size_t n, const char* format, VaList* arg) {
    return vsnprintf_ctx(s, n, format, arg);
}

int PS4_SYSV_ABI ps4_puts(const char* s) {
    return std::puts(s);
}

long PS4_SYSV_ABI ps4_ftell(FILE* stream) {
    return ftell(stream);
}

} // namespace Libraries::LibC
