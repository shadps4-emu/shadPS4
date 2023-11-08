#include "core/hle/libraries/libkernel/file_system.h"

#include <filesystem>

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
    const bool isDirectory = (flags & SCE_KERNEL_O_DIRECTORY) != 0;
    const bool create = (flags & SCE_KERNEL_O_CREAT) != 0;

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
        if (!std::filesystem::is_directory(file->m_host_name)) {  // directory doesn't exist
            if (create) {                                         // if we have a create flag create it
                /* std::error_code e;
                if (std::filesystem::create_directories(file->m_host_name,e)) {
                    return handle;
                } else {
                    std::string message =e.message();
                    return SCE_KERNEL_ERROR_ENOTDIR;
                }
                return SCE_KERNEL_ERROR_ENOTDIR;*/
                // there is seems to be a bug with create_directories return false even if the directory creates so don't check until we find
                // a better solution
                std::filesystem::create_directories(file->m_host_name);
                return handle;
            }
        } else {
            if (create) {
                return handle;  // directory already exists
            } else {
                file->dirents = Common::FS::File::getDirectoryEntries(file->m_host_name);
                file->dirents_index = 0;
            }
        }
    }
    file->isOpened = true;
    return handle;
}

int PS4_SYSV_ABI sceKernelClose(int handle) {
    LOG_INFO_IF(log_file_fs, "sceKernelClose descriptor = {}\n", handle);
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->getFile(handle);
    file->isOpened = false;
    h->deleteHandle(handle);
    return SCE_OK;
}

int PS4_SYSV_ABI sceKernelGetdents(int fd, char* buf, int nbytes) {
    PRINT_FUNCTION_NAME();
    // TODO error codes
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->getFile(fd);

    if (file->dirents_index == file->dirents.size()) {
        return 0;
    }

    const auto& entry = file->dirents.at(file->dirents_index++);
    auto str = entry.name;
    auto str_size = str.size() - 1;

    SceKernelDirent* sce_ent = (SceKernelDirent*)buf;
    sce_ent->d_fileno = fd;  // TODO this should be unique but atm it changes maybe switch to a hash or something?
    sce_ent->d_reclen = sizeof(SceKernelDirent);
    sce_ent->d_type = (entry.isFile ? 8 : 4);
    sce_ent->d_namlen = str_size;
    str.copy(sce_ent->d_name, SCE_MAX_PATH);

    return sizeof(SceKernelDirent);
}

int PS4_SYSV_ABI posix_open(const char* path, int flags, /* SceKernelMode*/ u16 mode) {
    LOG_INFO_IF(log_file_fs, "posix open redirect to sceKernelOpen\n");
    int result = sceKernelOpen(path, flags, mode);
    if (result < 0) {
        BREAKPOINT();  // posix calls different only for their return values
    }
    return result;
}

int PS4_SYSV_ABI posix_close(int handle) {
    LOG_INFO_IF(log_file_fs, "posix close redirect to sceKernelClose\n");
    int result = sceKernelClose(handle);
    if (result < 0) {
        BREAKPOINT();  // posix calls different only for their return values
    }
    return result;
}

void fileSystemSymbolsRegister(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("1G3lF1Gg1k8", "libkernel", 1, "libkernel", 1, 1, sceKernelOpen);
    LIB_FUNCTION("UK2Tl2DWUns", "libkernel", 1, "libkernel", 1, 1, sceKernelClose);
    LIB_FUNCTION("j2AIqSqJP0w", "libkernel", 1, "libkernel", 1, 1, sceKernelGetdents);
    LIB_FUNCTION("wuCroIGjt2g", "libScePosix", 1, "libkernel", 1, 1, posix_open);
    LIB_FUNCTION("bY-PO6JhzhQ", "libScePosix", 1, "libkernel", 1, 1, posix_close);
}

}  // namespace Core::Libraries::LibKernel
