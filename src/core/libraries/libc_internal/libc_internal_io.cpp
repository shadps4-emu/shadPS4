// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdarg>
#include <cstdio>

#include <common/va_ctx.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/libc_internal/libc_internal_io.h"
#include "core/libraries/libc_internal/libc_internal_threads.h"
#include "core/libraries/libs.h"
#include "printf.h"

namespace Libraries::LibcInternal {

s32 PS4_SYSV_ABI internal_snprintf(char* s, u64 n, VA_ARGS) {
    VA_CTX(ctx);
    return snprintf_ctx(s, n, &ctx);
}

std::vector<OrbisFILE*> g_files{};
// Constants for tracking accurate file indexes.
// Since the file struct is exposed to the application, accuracy is important.
static constexpr s32 g_initial_files = 5;
static constexpr s32 g_max_files = 0x100 - g_initial_files;

OrbisFILE* PS4_SYSV_ABI internal__Fofind() {
    u64 index = 0;
    while (index != g_max_files) {
        OrbisFILE* file = g_files.at(index);
        // If file doesn't exist, create it.
        if (file == nullptr) {
            file = new OrbisFILE();
            if (file == nullptr) {
                return nullptr;
            }
            // Store new file in the array, initialize default values, and return it.
            g_files.at(index) = file;
            file->_Mode = 0x80;
            file->_Idx = index + g_initial_files;
            return file;
        }
        // Special case, files with mode 0 are returned?
        if (file->_Mode == 0) {
            file->_Mode = 0xff7f;
            return file;
        }
        index++;
    }
    return nullptr;
}

void PS4_SYSV_ABI internal__Lockfilelock(OrbisFILE* file) {
    if (file != nullptr && file->_Mutex != nullptr) {
        internal__Mtxlock(&file->_Mutex);
    }
}

void PS4_SYSV_ABI internal__Unlockfilelock(OrbisFILE* file) {
    if (file != nullptr && file->_Mutex != nullptr) {
        internal__Mtxunlock(&file->_Mutex);
    }
}

OrbisFILE* PS4_SYSV_ABI internal__Foprep(const char* path, const char* mode, OrbisFILE* file,
                                         s32 fd, s32 s_mode, s32 flag) {
    if (file == nullptr) {
        *Kernel::__Error() = POSIX_ENOMEM;
    }

    // Preserve mode and index
    Libraries::Kernel::PthreadMutexT file_mtx = file->_Mutex;
    u8 file_index = file->_Idx;
    u16 file_mode = file->_Mode & 0x80;

    // Real library does a memcpy using a static global FILE object.
    // This stored file is just zeros, with the only exception being a handle of -1.
    memset(file, 0, sizeof(OrbisFILE));
    file->_Handle = -1;

    // Not sure what this magic is for, but I'll replicate it.
    u8* ptr = &file->_Cbuf;
    // Note: this field is supposed to be a pthread mutex.
    // Since we don't export pthread HLEs for other functions, I'll avoid handling this for now.
    file->_Mutex = nullptr;
    file->_Idx = file_index;
    file->_Buf = ptr;
    file->_Bend = &file->unk2;
    file->_Next = ptr;
    file->_Rend = ptr;
    file->_WRend = ptr;
    file->_Wend = ptr;
    file->_WWend = ptr;
    file->_Rback = ptr;
    file->_WRback = &file->unk1;

    // Parse inputted mode string
    char* mode_str = *mode;
    u16 calc_mode = 0;
    u16 access_mode = 0;
    if (mode_str[0] == 'r') {
        calc_mode = 1 | file_mode;
    } else if (mode_str[0] == 'w') {
        calc_mode = 0x1a | file_mode;
    } else if (mode_str[0] == 'a') {
        calc_mode = 0x16 | file_mode;
    } else {
        // Closes the file and returns EINVAL.
        file->_Mode = file_mode;
        if (flag == 0) {
            internal__Mtxinit(&file_mtx, nullptr);
        } else {
            file->_Mutex = file_mtx;
            internal__Unlockfilelock(file);
        }
        internal_fclose(file);
        *Kernel::__Error() = POSIX_EINVAL;
        return nullptr;
    }
    file->_Mode = calc_mode;

    do {
        // This is all basically straight from decomp, need to cleanup at some point.
        if (mode_str[1] == '+') {
            file_mode = 3;
            if ((~calc_mode & 3) == 0) {
                break;
            }
        } else if (mode_str[1] != 'b') {
            file_mode = 0x20;
            if ((calc_mode & 0x20) != 0) {
                break;
            }
        }
        mode_str++;
        calc_mode = file_mode | calc_mode;
        file->_Mode = calc_mode;
    } while (true);

    if (path == nullptr && fd >= 0) {
        // I guess this is for some internal behavior?
        file->_Handle = fd;
    } else {
        fd = internal__Fopen(path, calc_mode, s_mode == 0x55);
        file->_Handle = fd;
    }

    // Error case
    if (fd < 0) {
        // Closes the file, but ensures errno is unchanged.
        if (flag == 0) {
            internal__Mtxinit(&file_mtx, nullptr);
        } else {
            file->_Mutex = file_mtx;
            internal__Unlockfilelock(file);
        }
        s32 old_errno = *Kernel::__Error();
        internal_fclose(file);
        *Kernel::__Error() = old_errno;
        return nullptr;
    }

    if (flag == 0) {
        char mtx_name[0x20];
        std::snprintf(mtx_name, 0x20, "FileFD:0x%08X", fd);
        internal__Mtxinit(&file_mtx, mtx_name);
    } else {
        file->_Mutex = file_mtx;
    }
    return file;
}

s32 PS4_SYSV_ABI internal__Fopen(const char* path, u16 mode, bool flag) {
    u32 large_mode = mode;
    u16 open_mode = 0600;
    if (!flag) {
        open_mode = 0666;
    }
    // Straight from decomp, should probably get cleaned up at some point.
    s32 creat_flag = large_mode << 5 & 0x200;
    s32 excl_flag = large_mode << 5 & 0x800;
    s32 misc_flags = (large_mode & 8) * 0x80 + (large_mode & 4) * 2;
    // Real library has an array for this, where large_mode & 3 is used as an index.
    // That array has values [0, 0, 1, 2], so this call should match the result.
    s32 access_flag = std::max<s32>((large_mode & 3) - 1, 0);
    s32 open_flags = creat_flag | misc_flags | excl_flag | access_flag;
    return Libraries::Kernel::posix_open(path, open_flags, open_mode);
}

OrbisFILE* PS4_SYSV_ABI internal_fopen(const char* path, const char* mode) {
    std::scoped_lock lk{g_stream_mtx};
    OrbisFILE* file = internal__Fofind();
    return internal__Foprep(path, mode, file, -1, 0, 0);
}

s32 PS4_SYSV_ABI internal_fseek(OrbisFILE* stream, s64 offset, s32 whence) {
    return 0;
}

u64 PS4_SYSV_ABI internal_fread(void* ptr, u64 size, u64 nmemb, OrbisFILE* stream) {
    return 0;
}

s32 PS4_SYSV_ABI internal_fclose(OrbisFILE* stream) {
    return 0;
}

void RegisterlibSceLibcInternalIo(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("eLdDw6l0-bU", "libSceLibcInternal", 1, "libSceLibcInternal", internal_snprintf);
    LIB_FUNCTION("xGT4Mc55ViQ", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Fofind);
    LIB_FUNCTION("dREVnZkAKRE", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Foprep);
    LIB_FUNCTION("sQL8D-jio7U", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Fopen);
    LIB_FUNCTION("vZkmJmvqueY", "libSceLibcInternal", 1, "libSceLibcInternal",
                 internal__Lockfilelock);
    LIB_FUNCTION("0x7rx8TKy2Y", "libSceLibcInternal", 1, "libSceLibcInternal",
                 internal__Unlockfilelock);
}

void ForceRegisterlibSceLibcInternalIo(Core::Loader::SymbolsResolver* sym) {
    // Goal is to be minimally intrusive here to allow LLE for printf/stdout writes.
    LIB_FUNCTION("xeYO4u7uyJ0", "libSceLibcInternal", 1, "libSceLibcInternal", internal_fopen);
    LIB_FUNCTION("rQFVBXp-Cxg", "libSceLibcInternal", 1, "libSceLibcInternal", internal_fseek);
    LIB_FUNCTION("lbB+UlZqVG0", "libSceLibcInternal", 1, "libSceLibcInternal", internal_fread);
    LIB_FUNCTION("uodLYyUip20", "libSceLibcInternal", 1, "libSceLibcInternal", internal_fclose);
}

} // namespace Libraries::LibcInternal