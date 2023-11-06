#include "core/hle/libraries/libkernel/file_system.h"

#include "common/debug.h"
#include "common/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/hle/error_codes.h"
#include "core/hle/libraries/libs.h"

namespace Core::Libraries::LibKernel {

constexpr bool log_file_fs = true;  // disable it to disable logging

int PS4_SYSV_ABI sceKernelOpen(const char* path, int flags, u16 mode) {
    LOG_INFO_IF(log_file_fs, "sceKernelOpen path = {} flags = {:#x} mode = {:#x}\n", path, flags, mode);
    bool isDirectory = (flags & SCE_KERNEL_O_DIRECTORY) != 0;
    bool create = (flags & SCE_KERNEL_O_CREAT) != 0;

    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    u32 handle = h->createHandle();
    if (handle >= 2048) {  // max descriptor reached
        return SCE_KERNEL_ERROR_EMFILE;
    }
    auto* file = h->getFile(handle);
    if (isDirectory) {
        file->isDirectory = true;
        file->m_guest_name = path;
        file->m_host_name = mnt->getHostDirectory(file->m_guest_name);
    }

    return 0;
}

int PS4_SYSV_ABI posix_open(const char* path, int flags, /* SceKernelMode*/ u16 mode) {
    LOG_INFO_IF(log_file_fs, "posix open redirect to sceKernelOpen\n");
    int result = sceKernelOpen(path, flags, mode);
    if (result < 0) {
        BREAKPOINT();  // posix calls different only for their return values
    }
    return result;
}

void fileSystemSymbolsRegister(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("1G3lF1Gg1k8", "libkernel", 1, "libkernel", 1, 1, sceKernelOpen);
    LIB_FUNCTION("wuCroIGjt2g", "libScePosix", 1, "libkernel", 1, 1, posix_open);
}

}  // namespace Core::Libraries::LibKernel
