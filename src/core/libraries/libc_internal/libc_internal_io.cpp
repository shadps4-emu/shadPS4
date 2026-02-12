// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdarg>
#include <cstdio>
#include <map>

#include <common/va_ctx.h>
#include "common/alignment.h"
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

std::map<s32, OrbisFILE*> g_files{};
// Constants for tracking accurate file indexes.
// Since the file struct is exposed to the application, accuracy is important.
static constexpr s32 g_initial_files = 5;
static constexpr s32 g_max_files = 0x100;

OrbisFILE* PS4_SYSV_ABI internal__Fofind() {
    u64 index = g_initial_files;
    while (index != g_max_files) {
        OrbisFILE* file = g_files[index];
        // If file doesn't exist, create it.
        if (file == nullptr) {
            file = new OrbisFILE();
            if (file == nullptr) {
                return nullptr;
            }
            // Store new file in the array, initialize default values, and return it.
            g_files[index] = file;
            file->_Mode = 0x80;
            file->_Idx = index;
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
    Libraries::Kernel::PthreadMutexT mtx = file->_Mutex;
    Libraries::Kernel::PthreadMutexT* mtx_ptr = &file->_Mutex;
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
    const char* mode_str = mode;
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
            internal__Mtxinit(mtx_ptr, nullptr);
        } else {
            file->_Mutex = mtx;
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
            internal__Mtxinit(mtx_ptr, nullptr);
        } else {
            file->_Mutex = mtx;
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
        internal__Mtxinit(mtx_ptr, mtx_name);
    } else {
        file->_Mutex = mtx;
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
    std::scoped_lock lk{g_file_mtx};
    LOG_INFO(Lib_LibcInternal, "called, path {}, mode {}", path, mode);
    OrbisFILE* file = internal__Fofind();
    return internal__Foprep(path, mode, file, -1, 0, 0);
}

s32 PS4_SYSV_ABI internal_fflush(OrbisFILE* file) {
    if (file == nullptr) {
        std::scoped_lock lk{g_file_mtx};
        s32 fflush_result = 0;
        for (auto& file : g_files) {
            s32 res = internal_fflush(file.second);
            if (res < 0) {
                fflush_result = -1;
            }
        }
        return fflush_result;
    }
    if ((file->_Mode & 0x2000) != 0) {
        internal__Lockfilelock(file);
        u16 file_mode = file->_Mode;
        u8* file_buf_start = file->_Buf;
        u8* file_buf_end = file->_Next;
        while (file_buf_start < file_buf_end) {
            u64 size_to_write = static_cast<u64>(file_buf_end - file_buf_start);
            s32 write_bytes =
                Libraries::Kernel::sceKernelWrite(file->_Handle, file_buf_start, size_to_write);
            if (write_bytes < 1) {
                file_buf_start = file->_Buf;
                file->_Next = file_buf_start;
                file->_Wend = file_buf_start;
                file->_WWend = file_buf_start;
                u8* off_mode = reinterpret_cast<u8*>(&file->_Mode) + 1;
                *off_mode = *off_mode | 2;
                internal__Unlockfilelock(file);
                return -1;
            }
            file_buf_end = file->_Next;
            file_buf_start += write_bytes;
        }
        file->_Next = file_buf_start;
        file->_Wend = file_buf_start;
        file->_WWend = file_buf_start;
        file->_Mode = file_mode & 0xdfff;
        internal__Unlockfilelock(file);
    }
    return 0;
}

s64 PS4_SYSV_ABI internal__Nnl(OrbisFILE* file, u8* val1, u8* val2) {
    if (val1 < val2) {
        return val2 - val1;
    }
    return 0;
}

s32 PS4_SYSV_ABI internal__Fspos(OrbisFILE* file, Orbisfpos_t* file_pos, s64 offset, s32 whence) {
    if ((file->_Mode & 3) == 0) {
        return -1;
    }
    if (internal_fflush(file) != 0) {
        return -1;
    }
    if (whence >= 3) {
        *Libraries::Kernel::__Error() = POSIX_EINVAL;
        return -1;
    }
    if (file_pos != nullptr) {
        offset = offset + file_pos->_Off;
    }
    if (whence == 1 && (file->_Mode & 0x1000) != 0) {
        s64 val1 = internal__Nnl(file, file->_Rback, &file->_Cbuf);
        u8* rsave_ptr = file->_Rsave;
        if (rsave_ptr == nullptr) {
            rsave_ptr = file->_Rend;
        }
        s64 val2 = internal__Nnl(file, file->_Next, rsave_ptr);
        s64 val3 = internal__Nnl(file, file->_Next, file->_WRend);
        offset = offset - (val1 + val2 + val3);
    }
    s64 result = 0;
    if (whence == 2 || (whence == 1 && offset != 0) || (whence == 0 && offset != -1)) {
        result = Libraries::Kernel::posix_lseek(file->_Handle, offset, whence);
    }
    if (result == -1) {
        return -1;
    }

    u16 file_mode = file->_Mode;
    if ((file_mode & 0x3000) != 0) {
        u8* file_buf = file->_Buf;
        file->_Next = file_buf;
        file->_Rend = file_buf;
        file->_WRend = file_buf;
        file->_Wend = file_buf;
        file->_WWend = file_buf;
        file->_Rback = &file->_Cbuf;
        file->_WRback = &file->unk1;
        file->_Rsave = nullptr;
    }
    if (file_pos != nullptr) {
        std::memcpy(&file->_Wstate, &file_pos->_Wstate, sizeof(Orbis_Mbstatet));
    }
    file->_Mode = file_mode & 0xceff;
    return 0;
}

s32 PS4_SYSV_ABI internal_fseek(OrbisFILE* file, s64 offset, s32 whence) {
    internal__Lockfilelock(file);
    LOG_TRACE(Lib_LibcInternal, "called, file handle {:#x}, offset {:#x}, whence {:#x}",
              file->_Handle, offset, whence);
    s32 result = internal__Fspos(file, nullptr, offset, whence);
    internal__Unlockfilelock(file);
    return result;
}

s32 PS4_SYSV_ABI internal__Frprep(OrbisFILE* file) {
    if (file->_Rend > file->_Next) {
        return 1;
    }
    if ((file->_Mode & 0x100) != 0) {
        return 0;
    }
    u16 mode = file->_Mode;
    if ((mode & 0xa001) != 1) {
        // Lot of magic here, might be valuable to figure out what this does.
        file->_Mode = (((mode ^ 0x8000) >> 0xf) << 0xe) | mode | 0x200;
        return -1;
    }

    u8* file_buf = file->_Buf;
    if ((mode & 0x800) == 0 && file_buf == &file->_Cbuf) {
        // Allocate a new file buffer, for now, we'll use host malloc to create it.
        // When we have an HLE for malloc, that should be used instead.
        u8* new_buffer = std::bit_cast<u8*>(std::malloc(0x10000));
        if (new_buffer == nullptr) {
            file->_Buf = file_buf;
            file->_Bend = file_buf + 1;
        } else {
            file->_Mode = file->_Mode | 0x40;
            file->_Buf = new_buffer;
            file->_Bend = new_buffer + 0x10000;
            file->_WRend = new_buffer;
            file->_WWend = new_buffer;
            file_buf = new_buffer;
        }
    }
    file->_Next = file_buf;
    file->_Rend = file_buf;
    file->_Wend = file_buf;
    // Intentional shrinking here, library treats value as 32-bit.
    s32 read_result =
        Libraries::Kernel::sceKernelRead(file->_Handle, file_buf, file->_Bend - file_buf);
    if (read_result < 0) {
        u8* off_mode = reinterpret_cast<u8*>(&file->_Mode) + 1;
        *off_mode = *off_mode | 0x42;
        return -1;
    } else if (read_result != 0) {
        file->_Mode = file->_Mode | 0x5000;
        file->_Rend = file->_Rend + read_result;
        return 1;
    }
    file->_Mode = (file->_Mode & 0xaeff) | 0x4100;
    return 0;
}

u64 PS4_SYSV_ABI internal_fread(char* ptr, u64 size, u64 nmemb, OrbisFILE* file) {
    if (size == 0 || nmemb == 0) {
        return 0;
    }
    internal__Lockfilelock(file);
    LOG_TRACE(Lib_LibcInternal, "called, file handle {:#x}, size {:#x}, nmemb {:#x}", file->_Handle,
              size, nmemb);
    s64 total_size = size * nmemb;
    s64 remaining_size = total_size;
    if ((file->_Mode & 0x4000) != 0) {
        while (remaining_size != 0) {
            u8* rback_ptr = file->_Rback;
            if (&file->_Cbuf <= rback_ptr) {
                break;
            }
            file->_Rback = rback_ptr + 1;
            *ptr = *rback_ptr;
            ptr++;
            remaining_size--;
        }
    }

    while (remaining_size != 0) {
        u8* file_ptr = file->_Rsave;
        if (file_ptr == nullptr) {
            file_ptr = file->_Rend;
        } else {
            file->_Rend = file_ptr;
            file->_Rsave = nullptr;
        }
        u8* src = file->_Next;
        if (file_ptr <= src) {
            s32 res = internal__Frprep(file);
            if (res < 1) {
                internal__Unlockfilelock(file);
                return (total_size - remaining_size) / size;
            }
            src = file->_Next;
            file_ptr = file->_Rend;
        }
        u64 copy_bytes = std::min<u64>(file_ptr - src, remaining_size);
        std::memcpy(ptr, src, copy_bytes);
        file->_Next += copy_bytes;
        ptr += copy_bytes;
        remaining_size -= copy_bytes;
    }
    internal__Unlockfilelock(file);
    return (total_size - remaining_size) / size;
}

void PS4_SYSV_ABI internal__Fofree(OrbisFILE* file) {
    u8* cbuf_ptr = &file->_Cbuf;
    s8 trunc_mode = static_cast<s8>(file->_Mode);

    file->_Mode = 0;
    file->_Handle = -1;
    file->_Buf = cbuf_ptr;
    file->_Next = cbuf_ptr;
    file->_Rend = cbuf_ptr;
    file->_WRend = cbuf_ptr;
    file->_Wend = cbuf_ptr;
    file->_WWend = cbuf_ptr;
    file->_Rback = cbuf_ptr;
    file->_WRback = &file->unk1;
    if (trunc_mode < 0) {
        // Remove file from vector
        g_files.erase(file->_Idx);
        internal__Mtxdst(&file->_Mutex);
        free(file);
    }
}

s32 PS4_SYSV_ABI internal_fclose(OrbisFILE* file) {
    if (file == nullptr) {
        return -1;
    }

    LOG_INFO(Lib_LibcInternal, "called, file handle {:#x}", file->_Handle);
    if ((file->_Mode & 3) == 0 || file->_Handle < 0) {
        std::scoped_lock lk{g_file_mtx};
        internal__Fofree(file);
        *Libraries::Kernel::__Error() = POSIX_EBADF;
    } else {
        s32 fflush_result = internal_fflush(file);
        std::scoped_lock lk{g_file_mtx};
        if ((file->_Mode & 0x40) != 0) {
            std::free(file->_Buf);
        }
        file->_Buf = nullptr;
        s32 close_result = Libraries::Kernel::posix_close(file->_Handle);
        internal__Fofree(file);
        // Need to figure out what exactly this means.
        return ~-(close_result == 0) | fflush_result;
    }
    return 0;
}

void RegisterlibSceLibcInternalIo(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("eLdDw6l0-bU", "libSceLibcInternal", 1, "libSceLibcInternal", internal_snprintf);
    LIB_FUNCTION("MUjC4lbHrK4", "libSceLibcInternal", 1, "libSceLibcInternal", internal_fflush);
    LIB_FUNCTION("xGT4Mc55ViQ", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Fofind);
    LIB_FUNCTION("dREVnZkAKRE", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Foprep);
    LIB_FUNCTION("sQL8D-jio7U", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Fopen);
    LIB_FUNCTION("A+Y3xfrWLLo", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Fspos);
    LIB_FUNCTION("Ss3108pBuZY", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Nnl);
    LIB_FUNCTION("9s3P+LCvWP8", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Frprep);
    LIB_FUNCTION("jVDuvE3s5Bs", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Fofree);
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