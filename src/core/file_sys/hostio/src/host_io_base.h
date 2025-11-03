// INAA License @marecl 2025

#pragma once

#include <filesystem>
#include <system_error>

#include "common/types.h"
#include "core/file_sys/quasifs/quasi_types.h"
#include "core/libraries/kernel/file_system.h"

namespace HostIODriver {
namespace fs = std::filesystem;
using namespace QuasiFS;

class HostIO_Base {

protected:
public:
    HostIO_Base();
    ~HostIO_Base();

    //
    // Native wrappers
    //

    virtual s32 Open(const fs::path& path, int flags, u16 mode = 0755);
    virtual s32 Creat(const fs::path& path, u16 mode = 0755);
    virtual s32 Close(const int fd);

    virtual s32 LinkSymbolic(const fs::path& src, const fs::path& dst);
    virtual s32 Link(const fs::path& src, const fs::path& dst);
    virtual s32 Unlink(const fs::path& path);
    virtual s32 Flush(const int fd);
    virtual s32 FSync(const int fd);
    virtual s32 Truncate(const fs::path& path, u64 size);
    virtual s32 FTruncate(const int fd, u64 size);
    virtual s64 LSeek(const int fd, u64 offset, QuasiFS::SeekOrigin origin);
    virtual s64 Tell(const int fd);
    virtual s64 Write(const int fd, const void* buf, u64 count);
    virtual s64 PWrite(const int fd, const void* buf, u64 count, u64 offset);
    virtual s64 Read(const int fd, void* buf, u64 count);
    virtual s64 PRead(const int fd, void* buf, u64 count, u64 offset);
    virtual s32 MKDir(const fs::path& path, u16 mode = 0755);
    virtual s32 RMDir(const fs::path& path);

    virtual s32 Stat(const fs::path& path, Libraries::Kernel::OrbisKernelStat* statbuf);
    virtual s32 FStat(const int fd, Libraries::Kernel::OrbisKernelStat* statbuf);

    virtual s32 Chmod(const fs::path& path, u16 mode);
    virtual s32 FChmod(const int fd, u16 mode);
    //
    // Derived, complex functions are to be handled by main FS class
    //
};
} // namespace HostIODriver