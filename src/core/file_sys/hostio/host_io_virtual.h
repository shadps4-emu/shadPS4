// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2025

#pragma once

#include <mutex>

#include "core/file_sys/quasifs/quasi_types.h"
#include "core/libraries/kernel/file_system.h"

#include "src/host_io_base.h"

namespace HostIODriver {
class HostIO_Virtual final : public HostIO_Base {
protected:
    std::mutex ctx_mutex;
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
        std::lock_guard<std::mutex> lock(ctx_mutex);
        this->res = res;
        this->host_bound = host_bound;
        this->handle = handle;
    }

    void ClearCtx(void) {
        std::lock_guard<std::mutex> lock(ctx_mutex);
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
    s32 Open(const fs::path& path, s32 flags, u16 mode = 0755) override;
    s32 Creat(const fs::path& path, u16 mode = 0755) override;
    s32 Close(const s32 fd) override;

    s32 Link(const fs::path& src, const fs::path& dst) override;
    s32 LinkSymbolic(const fs::path& src, const fs::path& dst) override;
    s32 Unlink(const fs::path& path) override;
    s32 Remove(const fs::path& path) override;

    s32 Flush(const s32 fd) override;
    s32 FSync(const s32 fd) override;
    s64 LSeek(const s32 fd, s64 offset, s32 whence) override;
    s64 Tell(const s32 fd) override;

    s32 Truncate(const fs::path& path, u64 size) override;
    s32 FTruncate(const s32 fd, u64 size) override;

    s64 Read(const s32 fd, void* buf, u64 count) override;
    s64 PRead(const s32 fd, void* buf, u64 count, s64 offset) override;
    s64 ReadV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) override;
    s64 PReadV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt, s64 offset) override;

    s64 Write(const s32 fd, const void* buf, u64 count) override;
    s64 PWrite(const s32 fd, const void* buf, u64 count, s64 offset) override;
    s64 WriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) override;
    s64 PWriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt, s64 offset) override;

    s32 MKDir(const fs::path& path, u16 mode = 0755) override;
    s32 RMDir(const fs::path& path) override;

    s32 Stat(const fs::path& path, OrbisKernelStat* statbuf) override;
    s32 FStat(const s32 fd, OrbisKernelStat* statbuf) override;

    s32 Chmod(const fs::path& path, u16 mode) override;
    s32 FChmod(const s32 fd, u16 mode) override;

    s64 GetDents(const s32 fd, void* buf, u64 count, s64* basep) override;

    s32 Copy(const fs::path& src, const fs::path& dst, bool fail_if_exists) override;
    s32 Move(const fs::path& src, const fs::path& dst, bool fail_if_exists) override;
};
} // namespace HostIODriver