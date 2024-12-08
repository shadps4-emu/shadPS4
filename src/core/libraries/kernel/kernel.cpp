// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include <boost/asio/io_context.hpp>

#include "common/assert.h"
#include "common/debug.h"
#include "common/logging/log.h"
#include "common/polyfill_thread.h"
#include "common/thread.h"
#include "common/va_ctx.h"
#include "core/file_sys/fs.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/equeue.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/kernel/threads/exception.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/libs.h"

#ifdef _WIN64
#include <Rpc.h>
#endif
#include <common/singleton.h>

namespace Libraries::Kernel {

static u64 g_stack_chk_guard = 0xDEADBEEF54321ABC; // dummy return

boost::asio::io_context io_context;
static std::mutex m_asio_req;
static std::condition_variable_any cv_asio_req;
static std::atomic<u32> asio_requests;
static std::jthread service_thread;

void KernelSignalRequest() {
    std::unique_lock lock{m_asio_req};
    ++asio_requests;
    cv_asio_req.notify_one();
}

static void KernelServiceThread(std::stop_token stoken) {
    Common::SetCurrentThreadName("shadPS4:KernelServiceThread");

    while (!stoken.stop_requested()) {
        HLE_TRACE;
        {
            std::unique_lock lock{m_asio_req};
            Common::CondvarWait(cv_asio_req, lock, stoken, [] { return asio_requests != 0; });
        }
        if (stoken.stop_requested()) {
            break;
        }

        io_context.run();
        io_context.reset();

        asio_requests = 0;
    }
}

static PS4_SYSV_ABI void stack_chk_fail() {
    UNREACHABLE();
}

static thread_local int g_posix_errno = 0;

int* PS4_SYSV_ABI __Error() {
    return &g_posix_errno;
}

void ErrSceToPosix(int error) {
    g_posix_errno = error - ORBIS_KERNEL_ERROR_UNKNOWN;
}

int ErrnoToSceKernelError(int error) {
    return error + ORBIS_KERNEL_ERROR_UNKNOWN;
}

void SetPosixErrno(int e) {
    // Some error numbers are different between supported OSes or the PS4
    switch (e) {
    case EPERM:
        g_posix_errno = POSIX_EPERM;
        break;
    case EAGAIN:
        g_posix_errno = POSIX_EAGAIN;
        break;
    case ENOMEM:
        g_posix_errno = POSIX_ENOMEM;
        break;
    case EINVAL:
        g_posix_errno = POSIX_EINVAL;
        break;
    case ENOSPC:
        g_posix_errno = POSIX_ENOSPC;
        break;
    case ERANGE:
        g_posix_errno = POSIX_ERANGE;
        break;
    case EDEADLK:
        g_posix_errno = POSIX_EDEADLK;
        break;
    case ETIMEDOUT:
        g_posix_errno = POSIX_ETIMEDOUT;
        break;
    default:
        g_posix_errno = e;
    }
}

static uint64_t g_mspace_atomic_id_mask = 0;
static uint64_t g_mstate_table[64] = {0};

struct HeapInfoInfo {
    uint64_t size = sizeof(HeapInfoInfo);
    uint32_t flag;
    uint32_t getSegmentInfo;
    uint64_t* mspace_atomic_id_mask;
    uint64_t* mstate_table;
};

void PS4_SYSV_ABI sceLibcHeapGetTraceInfo(HeapInfoInfo* info) {
    info->mspace_atomic_id_mask = &g_mspace_atomic_id_mask;
    info->mstate_table = g_mstate_table;
    info->getSegmentInfo = 0;
}

s64 PS4_SYSV_ABI ps4__write(int d, const char* buf, std::size_t nbytes) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(d);
    if (file == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }
    std::scoped_lock lk{file->m_mutex};
    if (file->type == Core::FileSys::FileType::Device) {
        return file->device->write(buf, nbytes);
    }
    return file->f.WriteRaw<u8>(buf, nbytes);
}

s64 PS4_SYSV_ABI ps4__read(int d, void* buf, u64 nbytes) {
    if (d == 0) {
        return static_cast<s64>(
            strlen(std::fgets(static_cast<char*>(buf), static_cast<int>(nbytes), stdin)));
    }
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(d);
    if (file == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }
    std::scoped_lock lk{file->m_mutex};
    if (file->type == Core::FileSys::FileType::Device) {
        return file->device->read(buf, nbytes);
    }
    return file->f.ReadRaw<u8>(buf, nbytes);
}

struct OrbisKernelUuid {
    u32 timeLow;
    u16 timeMid;
    u16 timeHiAndVersion;
    u8 clockSeqHiAndReserved;
    u8 clockSeqLow;
    u8 node[6];
};

int PS4_SYSV_ABI sceKernelUuidCreate(OrbisKernelUuid* orbisUuid) {
#ifdef _WIN64
    UUID uuid;
    UuidCreate(&uuid);
    orbisUuid->timeLow = uuid.Data1;
    orbisUuid->timeMid = uuid.Data2;
    orbisUuid->timeHiAndVersion = uuid.Data3;
    orbisUuid->clockSeqHiAndReserved = uuid.Data4[0];
    orbisUuid->clockSeqLow = uuid.Data4[1];
    for (int i = 0; i < 6; i++) {
        orbisUuid->node[i] = uuid.Data4[2 + i];
    }
#else
    LOG_ERROR(Kernel, "sceKernelUuidCreate: Add linux");
#endif
    return 0;
}

int PS4_SYSV_ABI kernel_ioctl(int fd, u64 cmd, VA_ARGS) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        LOG_INFO(Lib_Kernel, "ioctl: fd = {:X} cmd = {:X} file == nullptr", fd, cmd);
        g_posix_errno = POSIX_EBADF;
        return -1;
    }
    if (file->type != Core::FileSys::FileType::Device) {
        LOG_WARNING(Lib_Kernel, "ioctl: fd = {:X} cmd = {:X} file->type != Device", fd, cmd);
        g_posix_errno = ENOTTY;
        return -1;
    }
    VA_CTX(ctx);
    int result = file->device->ioctl(cmd, &ctx);
    LOG_TRACE(Lib_Kernel, "ioctl: fd = {:X} cmd = {:X} result = {}", fd, cmd, result);
    if (result < 0) {
        ErrSceToPosix(result);
        return -1;
    }
    return result;
}

const char* PS4_SYSV_ABI sceKernelGetFsSandboxRandomWord() {
    const char* path = "sys";
    return path;
}

int PS4_SYSV_ABI posix_connect() {
    return -1;
}

int PS4_SYSV_ABI _sigprocmask() {
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_getpagesize() {
    return 16_KB;
}

void RegisterKernel(Core::Loader::SymbolsResolver* sym) {
    service_thread = std::jthread{KernelServiceThread};

    Libraries::Kernel::RegisterFileSystem(sym);
    Libraries::Kernel::RegisterTime(sym);
    Libraries::Kernel::RegisterThreads(sym);
    Libraries::Kernel::RegisterKernelEventFlag(sym);
    Libraries::Kernel::RegisterMemory(sym);
    Libraries::Kernel::RegisterEventQueue(sym);
    Libraries::Kernel::RegisterProcess(sym);
    Libraries::Kernel::RegisterException(sym);

    LIB_OBJ("f7uOxY9mM1U", "libkernel", 1, "libkernel", 1, 1, &g_stack_chk_guard);
    LIB_FUNCTION("PfccT7qURYE", "libkernel", 1, "libkernel", 1, 1, kernel_ioctl);
    LIB_FUNCTION("JGfTMBOdUJo", "libkernel", 1, "libkernel", 1, 1, sceKernelGetFsSandboxRandomWord);
    LIB_FUNCTION("XVL8So3QJUk", "libkernel", 1, "libkernel", 1, 1, posix_connect);
    LIB_FUNCTION("6xVpy0Fdq+I", "libkernel", 1, "libkernel", 1, 1, _sigprocmask);
    LIB_FUNCTION("Xjoosiw+XPI", "libkernel", 1, "libkernel", 1, 1, sceKernelUuidCreate);
    LIB_FUNCTION("Ou3iL1abvng", "libkernel", 1, "libkernel", 1, 1, stack_chk_fail);
    LIB_FUNCTION("9BcDykPmo1I", "libkernel", 1, "libkernel", 1, 1, __Error);
    LIB_FUNCTION("DRuBt2pvICk", "libkernel", 1, "libkernel", 1, 1, ps4__read);
    LIB_FUNCTION("k+AXqu2-eBc", "libkernel", 1, "libkernel", 1, 1, posix_getpagesize);
    LIB_FUNCTION("k+AXqu2-eBc", "libScePosix", 1, "libkernel", 1, 1, posix_getpagesize);
    LIB_FUNCTION("NWtTN10cJzE", "libSceLibcInternalExt", 1, "libSceLibcInternal", 1, 1,
                 sceLibcHeapGetTraceInfo);
    LIB_FUNCTION("FxVZqBAA7ks", "libkernel", 1, "libkernel", 1, 1, ps4__write);
    LIB_FUNCTION("FN4gaPmuFV8", "libScePosix", 1, "libkernel", 1, 1, ps4__write);
}

} // namespace Libraries::Kernel
