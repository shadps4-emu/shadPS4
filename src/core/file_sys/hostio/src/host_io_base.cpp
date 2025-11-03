// INAA License @marecl 2025

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
s32 HostIO_Base::Creat(const fs::path& path, u16 mode) { STUB() } 
s32 HostIO_Base::Close(const int fd) { STUB() } 

s32 HostIO_Base::Link(const fs::path& src, const fs::path& dst) { STUB() } 
s32 HostIO_Base::Unlink(const fs::path& path) { STUB() } 
s32 HostIO_Base::LinkSymbolic(const fs::path& src, const fs::path& dst) { STUB() } 

s32 HostIO_Base::Flush(const int fd) { STUB() } 
s32 HostIO_Base::FSync(const int fd) { STUB() } 
s64 HostIO_Base::LSeek(const int fd, u64 offset, QuasiFS::SeekOrigin origin) { STUB() } 
s64 HostIO_Base::Tell(const int fd) { STUB() } 

s32 HostIO_Base::Truncate(const fs::path& path, u64 size) { STUB() } 
s32 HostIO_Base::FTruncate(const int fd, u64 size) { STUB() } 

s64 HostIO_Base::Write(const int fd, const void* buf, u64 count) { STUB() } 
s64 HostIO_Base::Read(const int fd, void* buf, u64 count) { STUB() } 

s64 HostIO_Base::PWrite(const int fd, const void* buf, u64 count, u64 offset) { STUB() } 
s64 HostIO_Base::PRead(const int fd, void* buf, u64 count, u64 offset) { STUB() } 

s32 HostIO_Base::MKDir(const fs::path& path, u16 mode) { STUB() } 
s32 HostIO_Base::RMDir(const fs::path& path) { STUB() } 

s32 HostIO_Base::Stat(const fs::path& path, Libraries::Kernel::OrbisKernelStat* statbuf) { STUB() } 
s32 HostIO_Base::FStat(const int fd, Libraries::Kernel::OrbisKernelStat* statbuf) { STUB() } 

s32 HostIO_Base::Chmod(const fs::path& path, u16 mode) { STUB() } 
s32 HostIO_Base::FChmod(const int fd, u16 mode) { STUB() }
// clang-format on

} // namespace HostIODriver