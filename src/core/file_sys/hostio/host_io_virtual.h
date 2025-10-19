// INAA License @marecl 2025

#pragma once

#include "core/libraries/kernel/file_system.h"

#include "core/file_sys/quasifs/quasi_types.h"

#include "src/host_io_base.h"

namespace HostIODriver {
class HostIO_Virtual : public HostIO_Base {
protected:
    Resolved* res{nullptr};
    fd_handle_ptr handle{nullptr};
    bool host_bound{false};

public:
    HostIO_Virtual();
    ~HostIO_Virtual();

    //
    // Resolved holds necessary context to perform any FS operation
    // Set context immediately before target operation and clear immediately after
    // to avoid accidental mixups
    // Path is more of a... suggestion where we might be.
    //

    void SetCtx(Resolved* res, bool host_bound, fd_handle_ptr handle) {
        this->res = res;
        this->host_bound = host_bound;
        this->handle = handle;
    }

    void ClearCtx(void) {
        this->res = nullptr;
        this->handle = nullptr;
        this->host_bound = false;
    }

    //
    // Native wrappers
    //

    /**
     * WARNING
     * This function doesn't return numeric fd
     * The only outputs are 0 and -errno!
     */
    int Open(const fs::path& path, int flags, u16 mode = 0755) override;
    int Creat(const fs::path& path, u16 mode = 0755) override;
    int Close(const int fd) override;

    int LinkSymbolic(const fs::path& src, const fs::path& dst) override;
    int Link(const fs::path& src, const fs::path& dst) override;
    int Unlink(const fs::path& path) override;
    int Flush(const int fd) override;
    int FSync(const int fd) override;
    int Truncate(const fs::path& path, u64 size) override;
    int FTruncate(const int fd, u64 size) override;
    u64 LSeek(const int fd, u64 offset, QuasiFS::SeekOrigin origin) override;
    s64 Tell(const int fd) override;
    s64 Write(const int fd, const void* buf, u64 count) override;
    s64 PWrite(const int fd, const void* buf, u64 count, u64 offset) override;
    s64 Read(const int fd, void* buf, u64 count) override;
    s64 PRead(const int fd, void* buf, u64 count, u64 offset) override;
    int MKDir(const fs::path& path, u16 mode = 0755) override;
    int RMDir(const fs::path& path) override;

    int Stat(const fs::path& path, Libraries::Kernel::OrbisKernelStat* statbuf) override;
    int FStat(const int fd, Libraries::Kernel::OrbisKernelStat* statbuf) override;

    int Chmod(const fs::path& path, u16 mode) override;
    int FChmod(const int fd, u16 mode) override;

    //
    // Derived, complex functions are to be handled by main FS class
    //
};
} // namespace HostIODriver