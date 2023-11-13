#include "core/hle/libraries/libc/libc_stdio.h"

#include "common/debug.h"
#include "common/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"

namespace Core::Libraries::LibC {

constexpr bool log_file_libc = true;  // disable it to disable logging

int PS4_SYSV_ABI printf(VA_ARGS) {
    VA_CTX(ctx);
    return printf_ctx(&ctx);
}

int PS4_SYSV_ABI fprintf(FILE* file, VA_ARGS) {
    int fd = _fileno(file);
    if (fd == 1 || fd == 2) {  // output stdout and stderr to console
        VA_CTX(ctx);
        return printf_ctx(&ctx);
    }
    LOG_ERROR_IF(log_file_libc, "libc:Unimplemented fprintf case\n");
    BREAKPOINT();
    return 0;
}

int PS4_SYSV_ABI snprintf(char* s, size_t n, VA_ARGS) {
    VA_CTX(ctx);
    return snprintf_ctx(s, n, &ctx);
}

int PS4_SYSV_ABI sprintf(char* s, VA_ARGS) {
    VA_CTX(ctx);
    return sprintf_ctx(s, &ctx);
}

int PS4_SYSV_ABI vsnprintf(char* s, size_t n, const char* format, VaList* arg) { return vsnprintf_ctx(s, n, format, arg); }

int PS4_SYSV_ABI puts(const char* s) { return std::puts(s); }

FILE* PS4_SYSV_ABI fopen(const char* filename, const char* mode) {
    LOG_ERROR_IF(log_file_libc, "fopen filename={} , mode ={}\n", filename, mode);
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    u32 handle = h->createHandle();
    auto* file = h->getFile(handle);
    file->m_guest_name = filename;
    file->m_host_name = mnt->getHostFile(file->m_guest_name);
    FILE* f = std::fopen(file->m_host_name.c_str(), mode);
    return f;
}
int PS4_SYSV_ABI fclose(FILE* stream) {
    LOG_ERROR_IF(log_file_libc, "fclose\n");
    if (stream != nullptr) {
        std::fclose(stream);
    }
    return 0;
}
int PS4_SYSV_ABI setvbuf(FILE* stream, char* buffer, int mode, size_t size) { return std::setvbuf(stream, buffer, mode, size); }
int PS4_SYSV_ABI fseek(FILE* stream, long int offset, int origin) { return std::fseek(stream, offset, origin); }
int PS4_SYSV_ABI fgetpos(FILE* stream, fpos_t* pos) { return std::fgetpos(stream, pos); }
size_t PS4_SYSV_ABI fread(void* ptr, size_t size, size_t count, FILE* stream) { return std::fread(ptr, size, count, stream); }
}  // namespace Core::Libraries::LibC
