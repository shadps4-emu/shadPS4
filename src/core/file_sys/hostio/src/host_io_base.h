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
using namespace Libraries::Kernel;

class HostIO_Base {
public:
    HostIO_Base();
    ~HostIO_Base();

    //
    // Native wrappers
    //

    virtual s32 Open(const fs::path& path, int flags, u16 mode = 0755);
    virtual s32 Creat(const fs::path& path, u16 mode = 0755);
    virtual s32 Close(const s32 fd);

    virtual s32 Link(const fs::path& src, const fs::path& dst);
    virtual s32 LinkSymbolic(const fs::path& src, const fs::path& dst);
    virtual s32 Unlink(const fs::path& path);
    virtual s32 Remove(const fs::path& path);

    virtual s32 Flush(const s32 fd);
    virtual s32 FSync(const s32 fd);
    virtual s32 Truncate(const fs::path& path, u64 size);
    virtual s32 FTruncate(const s32 fd, u64 size);
    virtual s64 LSeek(const s32 fd, s64 offset, s32 whence);
    virtual s64 Tell(const s32 fd);

    virtual s64 Read(const s32 fd, void* buf, u64 count);
    virtual s64 PRead(const s32 fd, void* buf, u64 count, s64 offset);
    virtual s64 ReadV(const s32 fd, OrbisKernelIovec* iov, u32 iovcnt);
    virtual s64 PReadV(const s32 fd, OrbisKernelIovec* iov, u32 iovcnt, s64 offset);

    virtual s64 Write(const s32 fd, const void* buf, u64 count);
    virtual s64 PWrite(const s32 fd, const void* buf, u64 count, s64 offset);
    virtual s64 WriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt);
    virtual s64 PWriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt, s64 offset);

    virtual s32 MKDir(const fs::path& path, u16 mode = 0755);
    virtual s32 RMDir(const fs::path& path);

    virtual s32 Stat(const fs::path& path, OrbisKernelStat* statbuf);
    virtual s32 FStat(const s32 fd, OrbisKernelStat* statbuf);

    virtual s32 Chmod(const fs::path& path, u16 mode);
    virtual s32 FChmod(const s32 fd, u16 mode);

    virtual s64 GetDents(const s32 fd, void* buf, u64 count, s64* basep);

    virtual s32 Copy(const fs::path& src, const fs::path& dst, bool fail_if_exists);
    virtual s32 Move(const fs::path& src, const fs::path& dst, bool fail_if_exists);
    s32 Rename(const fs::path& src, const fs::path& dst_name, bool fail_if_exists);
    //
    // Derived, complex functions are to be handled by main FS class
    //
};
} // namespace HostIODriver