// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#include <filesystem>

#include "common/logging/log.h"

#include "host_io_base.h"

#define STUB()                                                                                     \
    LOG_ERROR(Kernel_Fs, "Stub called in HostIO_Base: {}:{}", __FILE__, __LINE__);                 \
    return -38;

namespace HostIODriver {

HostIO_Base::HostIO_Base() = default;
HostIO_Base::~HostIO_Base() = default;

// clang-format off
s32 HostIO_Base::Open(const fs::path& path, int flags, u16 mode) { STUB(); }
s32 HostIO_Base::Creat(const fs::path& path, u16 mode) { STUB(); }
s32 HostIO_Base::Close(const s32 fd) { STUB(); }

s32 HostIO_Base::Link(const fs::path& src, const fs::path& dst) { STUB(); }
s32 HostIO_Base::LinkSymbolic(const fs::path& src, const fs::path& dst) { STUB(); }
s32 HostIO_Base::Unlink(const fs::path& path) { STUB(); }
s32 HostIO_Base::Remove(const fs::path& path) { STUB(); }

s32 HostIO_Base::Flush(const s32 fd) { STUB(); }
s32 HostIO_Base::FSync(const s32 fd) { STUB(); }
s64 HostIO_Base::LSeek(const s32 fd, s64 offset, s32 whence) { STUB(); }
s64 HostIO_Base::Tell(const s32 fd) { STUB(); }

s32 HostIO_Base::Truncate(const fs::path& path, u64 size) { STUB(); }
s32 HostIO_Base::FTruncate(const s32 fd, u64 size) { STUB(); }

s64 HostIO_Base::Read(const s32 fd, void* buf, u64 count) { STUB(); }
s64 HostIO_Base::PRead(const s32 fd, void* buf, u64 count, s64 offset) { STUB(); }
s64 HostIO_Base::ReadV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) { STUB(); }
s64 HostIO_Base::PReadV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt,
                        s64 offset) { STUB(); }

s64 HostIO_Base::Write(const s32 fd, const void* buf, u64 count) { STUB(); }
s64 HostIO_Base::PWrite(const s32 fd, const void* buf, u64 count, s64 offset) { STUB(); }
s64 HostIO_Base::WriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt) { STUB(); }
s64 HostIO_Base::PWriteV(const s32 fd, const OrbisKernelIovec* iov, u32 iovcnt,
                         s64 offset) { STUB(); }

s32 HostIO_Base::MKDir(const fs::path& path, u16 mode) { STUB(); }
s32 HostIO_Base::RMDir(const fs::path& path) { STUB(); }

s32 HostIO_Base::Stat(const fs::path& path, OrbisKernelStat* statbuf) { STUB(); }
s32 HostIO_Base::FStat(const s32 fd, OrbisKernelStat* statbuf) { STUB(); }

s32 HostIO_Base::Chmod(const fs::path& path, u16 mode) { STUB(); }
s32 HostIO_Base::FChmod(const s32 fd, u16 mode) { STUB(); }

s64 HostIO_Base::GetDents(const s32 fd, void* buf, u64 count, s64* basep) { STUB(); }
// clang-format on

s32 HostIO_Base::Copy(const fs::path& src, const fs::path& dst, bool fail_if_exists) {
    std::error_code ec{};
    fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    if (ec)
        return -ec.value();

    return 0;
}

s32 HostIO_Base::Move(const fs::path& src, const fs::path& dst, bool fail_if_exists) {
    STUB();
}

} // namespace HostIODriver