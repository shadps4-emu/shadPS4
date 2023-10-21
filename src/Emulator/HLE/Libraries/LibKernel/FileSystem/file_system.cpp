#include "file_system.h"

#include <Core/PS4/HLE/ErrorCodes.h>
#include <Emulator/Util/singleton.h>
#include <Util/log.h>
#include <debug.h>

#include "Emulator/Host/fs.h"

namespace Emulator::HLE::Libraries::LibKernel::FileSystem {
constexpr bool log_file_fs = true;  // disable it to disable logging

int PS4_SYSV_ABI sceKernelOpen(const char* path, int flags, u16 mode) {
    LOG_INFO_IF(log_file_fs, "sceKernelOpen path = {} flags = {} mode = {}\n", path, log_hex_full(flags), log_hex_full(mode));

    bool isDirectory = (flags & SCE_KERNEL_O_DIRECTORY) != 0;
    bool create = (flags & SCE_KERNEL_O_CREAT) != 0;

    auto* h = singleton<Emulator::Host::Fs::HandleTable>::instance();
    auto* mnt = singleton<Emulator::Host::Fs::MntPoints>::instance();
    u32 handle = 0;
    if (h->createHandle().has_value()) {
        handle = h->createHandle().value();
    } else {
        return SCE_KERNEL_ERROR_EMFILE;  // file descriptors is full error(?)
    }
    Emulator::Host::Fs::File* file = h->getFile(handle);

    file->guest_path = path;
    if (isDirectory) {
        file->host_path = mnt->getHostDirectory(path);
        if (!std::filesystem::is_directory(file->host_path)) {  // directory doesn't exist
            if (create) {                                      // if we have a create flag create it
                if (std::filesystem::create_directories(file->host_path)) {
                    return handle;
                } else {
                    return SCE_KERNEL_ERROR_ENOTDIR;
                }
            } else {
                return SCE_KERNEL_ERROR_ENOTDIR;
            }
        }
    }
    return handle;
}

int PS4_SYSV_ABI sceKernelClose(int handle) {
    LOG_INFO_IF(log_file_fs, "sceKernelClose descriptor = {}\n", handle);
    auto* h = singleton<Emulator::Host::Fs::HandleTable>::instance();
    Emulator::Host::Fs::File* file = h->getFile(handle);
    file->valid = false;
    h->freeHandle(handle);
    return SCE_OK;
}
}  // namespace Emulator::HLE::Libraries::LibKernel::FileSystem