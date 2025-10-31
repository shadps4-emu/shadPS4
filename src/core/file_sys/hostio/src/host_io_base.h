// INAA License @marecl 2025

#pragma once

#include <filesystem>
#include <system_error>

#include "core/libraries/kernel/file_system.h"

#include "../../quasifs/quasi_types.h"
#include "common/types.h"

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

    virtual int Open(const fs::path& path, int flags, u16 mode = 0755);
    virtual int Creat(const fs::path& path, u16 mode = 0755);
    virtual int Close(const int fd);

    virtual int LinkSymbolic(const fs::path& src, const fs::path& dst);
    virtual int Link(const fs::path& src, const fs::path& dst);
    virtual int Unlink(const fs::path& path);
    virtual int Flush(const int fd);
    virtual int FSync(const int fd);
    virtual int Truncate(const fs::path& path, u64 size);
    virtual int FTruncate(const int fd, u64 size);
    virtual u64 LSeek(const int fd, u64 offset, QuasiFS::SeekOrigin origin);
    virtual s64 Tell(const int fd);
    virtual s64 Write(const int fd, const void* buf, u64 count);
    virtual s64 PWrite(const int fd, const void* buf, u64 count, u64 offset);
    virtual s64 Read(const int fd, void* buf, u64 count);
    virtual s64 PRead(const int fd, void* buf, u64 count, u64 offset);
    virtual int MKDir(const fs::path& path, u16 mode = 0755);
    virtual int RMDir(const fs::path& path);

    virtual int Stat(const fs::path& path, Libraries::Kernel::OrbisKernelStat* statbuf);
    virtual int FStat(const int fd, Libraries::Kernel::OrbisKernelStat* statbuf);

    virtual int Chmod(const fs::path& path, u16 mode);
    virtual int FChmod(const int fd, u16 mode);
    //
    // Derived, complex functions are to be handled by main FS class
    //
};
} // namespace HostIODriver