#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibKernel {
constexpr int SCE_MAX_PATH = 255;

struct SceKernelDirent {
    uint32_t d_fileno;             /* file number of entry */
    uint16_t d_reclen;             /* length of this record */
    uint8_t d_type;                /* file type, see below */
    uint8_t d_namlen;              /* length of string in d_name */
    char d_name[SCE_MAX_PATH + 1]; /* name must be no longer than this */
};

// Open flags
constexpr u32 SCE_KERNEL_O_RDONLY = 0x0000;         // Open as read-only
constexpr u32 SCE_KERNEL_O_WRONLY = 0x0001;         // Open as write-only
constexpr u32 SCE_KERNEL_O_RDWR = 0x0002;           // Open for reading and writing
constexpr u32 SCE_KERNEL_O_NONBLOCK = 0x0004;       // Perform non - blocking operation
constexpr u32 SCE_KERNEL_O_APPEND = 0x0008;         // Write by appending to the end of the file
constexpr u32 SCE_KERNEL_O_FSYNC = 0x0080;          // Perform synchronized writing
constexpr u32 SCE_KERNEL_O_SYNC = 0x0080;           // Perform synchronized writing
constexpr u32 SCE_KERNEL_O_CREAT = 0x0200;          // Create a file(overwrite if it already exists)
constexpr u32 SCE_KERNEL_O_TRUNC = 0x0400;          // Truncate the file size to 0(discard data if it already exists)
constexpr u32 SCE_KERNEL_O_EXCL = 0x0800;           // Error will occur if the file to create already exists
constexpr u32 SCE_KERNEL_O_DSYNC = 0x1000;          // Perform synchronized writing of the file content
constexpr u32 SCE_KERNEL_O_DIRECT = 0x00010000;     // Use cache as little as possible
constexpr u32 SCE_KERNEL_O_DIRECTORY = 0x00020000;  // Error will occur if not a directory

int PS4_SYSV_ABI sceKernelOpen(const char *path, int flags, /* SceKernelMode*/ u16 mode);
int PS4_SYSV_ABI sceKernelClose(int handle);
int PS4_SYSV_ABI sceKernelGetdents(int fd, char *buf, int nbytes);
int PS4_SYSV_ABI posix_open(const char *path, int flags, /* SceKernelMode*/ u16 mode);
int PS4_SYSV_ABI posix_close(int handle);

void fileSystemSymbolsRegister(Loader::SymbolsResolver *sym);

}  // namespace Core::Libraries::LibKernel
