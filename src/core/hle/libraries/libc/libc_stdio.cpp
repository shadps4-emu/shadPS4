#include "core/hle/libraries/libc/libc_stdio.h"

#include <core/file_sys/fs.h>

#include "common/debug.h"
#include "common/log.h"
#include "common/singleton.h"

namespace Core::Libraries::LibC {

constexpr bool log_file_libc = true;  // disable it to disable logging

int PS4_SYSV_ABI ps4_printf(VA_ARGS) {
    VA_CTX(ctx);
    return printf_ctx(&ctx);
}

int PS4_SYSV_ABI ps4_fprintf(FILE* file, VA_ARGS) {
    int fd = _fileno(file);
    if (fd == 1 || fd == 2) {  // output stdout and stderr to console
        VA_CTX(ctx);
        return printf_ctx(&ctx);
    }
    LOG_ERROR_IF(log_file_libc, "libc:Unimplemented fprintf case\n");
    BREAKPOINT();
    return 0;
}

int PS4_SYSV_ABI ps4_vsnprintf(char* s, size_t n, const char* format, VaList* arg) { return vsnprintf_ctx(s, n, format, arg); }

int PS4_SYSV_ABI ps4_puts(const char* s) { 
    return std::puts(s); 
}

FILE* PS4_SYSV_ABI ps4_fopen(const char* filename, const char* mode) {
    LOG_INFO_IF(log_file_libc, "fopen filename={} , mode ={}\n", filename, mode);
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    u32 handle = h->createHandle();
    auto* file = h->getFile(handle);
    //file->m_mutex.lock();
    file->m_guest_name = filename;
    file->m_host_name = mnt->getHostFile(file->m_guest_name);
    if (!file->f.open(file->m_host_name.c_str(), Common::FS::OpenMode::Read)) {
        LOG_ERROR_IF(log_file_libc, "fopen can't open file={}\n", filename);
    }
    FILE* descr = file->f.fileDescr();
    int handle1 = _fileno(descr);
    file->isOpened = true;
    //file->m_mutex.unlock();
    return descr;
}

int PS4_SYSV_ABI ps4_fseek(FILE* stream, long int offset, int origin) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    int handle = _fileno(stream);
    auto* file = h->getFile(handle - 2);
    file->m_mutex.lock();
    int seek = 0;
    switch (origin) {
        case 0: seek = file->f.seek(offset, Common::FS::SeekMode::Set); break;
        case 1: seek = file->f.seek(offset, Common::FS::SeekMode::Cur); break;
        case 2: seek = file->f.seek(offset, Common::FS::SeekMode::End); break;
    }
    file->m_mutex.unlock();
    return seek;
}

long PS4_SYSV_ABI ps4_ftell(FILE* stream) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    int handle = _fileno(stream);
    auto* file = h->getFile(handle - 2);
    //file->m_mutex.lock();
    long ftell = file->f.tell();
    //file->m_mutex.unlock();
    return ftell;
}

size_t PS4_SYSV_ABI ps4_fread(void* ptr, size_t size, size_t count, FILE* stream) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    int handle = _fileno(stream);
    auto* file = h->getFile(handle - 2);
    file->m_mutex.lock();
    u64 bytes_read = 0;
    file->f.read(ptr, size,count, &bytes_read);
    file->m_mutex.unlock();
    return bytes_read;
}

int PS4_SYSV_ABI ps4_fclose(FILE* stream) {
    LOG_INFO_IF(log_file_libc, "fclose\n");
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    int handle = _fileno(stream);
    auto* file = h->getFile(handle - 2);
    //file->m_mutex.lock();
    if (file->isOpened) {
        file->f.close();
    }
    h->deleteHandle(handle-2);
    //file->m_mutex.unlock();
    return 0;
}

}  // namespace Core::Libraries::LibC
