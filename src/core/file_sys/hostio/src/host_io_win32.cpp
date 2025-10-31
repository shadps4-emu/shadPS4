// INAA License @marecl 2025


#include "common/logging/log.h"

#include "../host_io_win32.h"

namespace HostIODriver {

HostIO_Win32::HostIO_Win32() = default;
HostIO_Win32::~HostIO_Win32() = default;



int HostIO_Win32::Creat(const fs::path& path, u16 mode) {
    LOG_ERROR(Kernel_Fs, "Stub called in HostIO_Base: {}:{}", __FILE__, __LINE__);
    return -38;
}
int HostIO_Win32::Open(const fs::path& path, int flags, u16 mode) {
    LOG_ERROR(Kernel_Fs, "Stub called in HostIO_Base: {}:{}", __FILE__, __LINE__);
    return -38;
}
int HostIO_Win32::Close(const int fd) {
    LOG_ERROR(Kernel_Fs, "Stub called in HostIO_Base: {}:{}", __FILE__, __LINE__);
    return -38;
}

} // namespace HostIODriver