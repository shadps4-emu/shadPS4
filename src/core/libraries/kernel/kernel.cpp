// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <boost/asio/io_context.hpp>

#include "common/assert.h"
#include "common/debug.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/polyfill_thread.h"
#include "common/thread.h"
#include "common/va_ctx.h"
#include "core/file_sys/fs.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/debug.h"
#include "core/libraries/kernel/equeue.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/kernel/threads/exception.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/sys_net.h"
#include "core/linker.h"
#include "core/tls.h"
#include "core/memory.h"

#ifdef _WIN64
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif
#include <common/singleton.h>
#include <core/libraries/network/net_error.h>
#include <core/libraries/network/sockets.h>
#include <core/linker.h>
#include "aio.h"

namespace Libraries::Kernel {

static u64 g_stack_chk_guard = 0xDEADBEEF54321ABC; // dummy return
static u64 g_thread_atexit_count_callback = 0;
static u64 g_thread_atexit_report_callback = 0;
static u64 g_thread_dtors_callback = 0;
static std::atomic<s32> g_thread_atexit_count = 0;
static u64 g_coredump_handler = 0;
static u64 g_coredump_handler_context = 0;
static std::atomic<u32> g_next_apr_submission_id = 1;
static std::mutex g_apr_submission_mutex;
static std::unordered_map<u32, VAddr> g_apr_submissions;
static thread_local std::array<u8, 0x68> g_sanitizer_new_replace_fallback{};

boost::asio::io_context io_context;
static std::mutex m_asio_req;
static std::condition_variable_any cv_asio_req;
static std::atomic<u32> asio_requests;
static std::jthread service_thread;

Core::EntryParams entry_params{};

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
        io_context.restart();

        asio_requests = 0;
    }
}

static PS4_SYSV_ABI void stack_chk_fail() {
    UNREACHABLE();
}

static thread_local s32 g_posix_errno = 0;

s32* PS4_SYSV_ABI __Error() {
    return &g_posix_errno;
}

void ErrSceToPosix(s32 error) {
    g_posix_errno = error - ORBIS_KERNEL_ERROR_UNKNOWN;
}

s32 ErrnoToSceKernelError(s32 error) {
    return error + ORBIS_KERNEL_ERROR_UNKNOWN;
}

s32 PS4_SYSV_ABI sceKernelError(s32 posix_error) {
    if (posix_error == 0) {
        return 0;
    }
    return posix_error + ORBIS_KERNEL_ERROR_UNKNOWN;
}

void SetPosixErrno(s32 e) {
    // Some error numbers are different between supported OSes
    switch (e) {
    case EPERM:
        g_posix_errno = POSIX_EPERM;
        break;
    case ENOENT:
        g_posix_errno = POSIX_ENOENT;
        break;
    case EDEADLK:
        g_posix_errno = POSIX_EDEADLK;
        break;
    case ENOMEM:
        g_posix_errno = POSIX_ENOMEM;
        break;
    case EACCES:
        g_posix_errno = POSIX_EACCES;
        break;
    case EFAULT:
        g_posix_errno = POSIX_EFAULT;
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
    case EAGAIN:
        g_posix_errno = POSIX_EAGAIN;
        break;
    case ETIMEDOUT:
        g_posix_errno = POSIX_ETIMEDOUT;
        break;
    default:
        LOG_WARNING(Kernel, "Unhandled errno {}", e);
        g_posix_errno = e;
    }
}

static u64 g_mspace_atomic_id_mask = 0;
static u64 g_mstate_table[64] = {0};

struct HeapInfoInfo {
    u64 size = sizeof(HeapInfoInfo);
    u32 flag;
    u32 getSegmentInfo;
    u64* mspace_atomic_id_mask;
    u64* mstate_table;
};

void PS4_SYSV_ABI sceLibcHeapGetTraceInfo(HeapInfoInfo* info) {
    info->mspace_atomic_id_mask = &g_mspace_atomic_id_mask;
    info->mstate_table = g_mstate_table;
    info->getSegmentInfo = 0;
}

struct OrbisKernelUuid {
    u32 timeLow;
    u16 timeMid;
    u16 timeHiAndVersion;
    u8 clockSeqHiAndReserved;
    u8 clockSeqLow;
    u8 node[6];
};
static_assert(sizeof(OrbisKernelUuid) == 0x10);

s32 PS4_SYSV_ABI sceKernelUuidCreate(OrbisKernelUuid* orbisUuid) {
    if (!orbisUuid) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
#ifdef _WIN64
    UUID uuid;
    if (UuidCreate(&uuid) != RPC_S_OK) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
#else
    uuid_t uuid;
    uuid_generate(uuid);
#endif
    std::memcpy(orbisUuid, &uuid, sizeof(OrbisKernelUuid));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI kernel_ioctl(s32 fd, u64 cmd, VA_ARGS) {
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
    s32 result = file->device->ioctl(cmd, &ctx);
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

s32 PS4_SYSV_ABI _sigprocmask() {
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_getpagesize() {
    return 16_KB;
}

// stubbed on non-devkit consoles
s32 PS4_SYSV_ABI sceKernelGetGPI() {
    LOG_DEBUG(Kernel, "called");
    return ORBIS_OK;
}

// stubbed on non-devkit consoles
s32 PS4_SYSV_ABI sceKernelSetGPO() {
    LOG_DEBUG(Kernel, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelGetAllowedSdkVersionOnSystem(s32* ver) {
    if (ver == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    // Returns the highest game SDK version this PS4 allows.
    *ver = CURRENT_FIRMWARE_VERSION | 0xfff;
    LOG_INFO(Lib_Kernel, "called, returned sw version: {}", *ver);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelGetSystemSwVersion(SwVersionStruct* ret) {
    if (ret == nullptr) {
        return ORBIS_OK;
    }
    u32 fake_fw = CURRENT_FIRMWARE_VERSION;
    ret->hex_representation = fake_fw;
    std::snprintf(ret->text_representation, 28, "%2x.%03x.%03x", fake_fw >> 0x18,
                  fake_fw >> 0xc & 0xfff, fake_fw & 0xfff);
    LOG_INFO(Lib_Kernel, "called, returned sw version: {}", ret->text_representation);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getargc() {
    return entry_params.argc;
}

const char** PS4_SYSV_ABI getargv() {
    return entry_params.argv;
}

s32 PS4_SYSV_ABI get_authinfo(s32 pid, AuthInfoData* p2) {
    LOG_WARNING(Lib_Kernel, "(STUBBED) called, pid: {}", pid);
    if (p2 == nullptr) {
        *Kernel::__Error() = POSIX_EPERM;
        return -1;
    }
    if (pid != 0 && pid != GLOBAL_PID) {
        *Kernel::__Error() = POSIX_ESRCH;
        return -1;
    }

    *p2 = {};
    p2->caps[0] = 0x2000000000000000;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelGetAppInfo(s32 pid, OrbisKernelAppInfo* app_info) {
    LOG_WARNING(Lib_Kernel, "(STUBBED) called, pid: {}", pid);
    if (pid != GLOBAL_PID) {
        return ORBIS_KERNEL_ERROR_EPERM;
    }
    if (app_info == nullptr) {
        return ORBIS_OK;
    }

    auto& game_info = Common::ElfInfo::Instance();
    *app_info = {};
    app_info->has_param_sfo = 1;
    strncpy(app_info->cusa_name, game_info.GameSerial().data(), 10);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _sceKernelSetThreadAtexitCount(u64 callback) {
    g_thread_atexit_count_callback = callback;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _sceKernelSetThreadAtexitReport(u64 callback) {
    g_thread_atexit_report_callback = callback;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _sceKernelSetThreadDtors(u64 callback) {
    g_thread_dtors_callback = callback;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _sceKernelRtldThreadAtexitIncrement() {
    ++g_thread_atexit_count;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _sceKernelRtldThreadAtexitDecrement() {
    --g_thread_atexit_count;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCoredumpRegisterCoredumpHandler(u64 handler, u64 context) {
    g_coredump_handler = handler;
    g_coredump_handler_context = context;
    return ORBIS_OK;
}

static bool IsValidGuestRange(VAddr addr, u64 size) {
    if (size == 0) {
        return true;
    }
    return addr != 0 && Core::Memory::Instance()->IsValidMapping(addr, size);
}

static u32 AllocateAprSubmissionId(VAddr command_buffer) {
    u32 id = g_next_apr_submission_id.fetch_add(1);
    if (id == 0) {
        id = g_next_apr_submission_id.fetch_add(1);
    }
    std::scoped_lock lock{g_apr_submission_mutex};
    g_apr_submissions[id] = command_buffer;
    return id;
}

static s32 WriteAprResult(void* result) {
    if (result == nullptr) {
        return ORBIS_OK;
    }
    const auto addr = reinterpret_cast<VAddr>(result);
    if (!IsValidGuestRange(addr, sizeof(u64))) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    *reinterpret_cast<u64*>(result) = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAprSubmitCommandBufferAndGetResult(void* command_buffer, u64,
                                                             void* result,
                                                             u32* out_submission_id) {
    if (!Core::IsGlobalPs5RuntimeMode() || command_buffer == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    const u32 id = AllocateAprSubmissionId(reinterpret_cast<VAddr>(command_buffer));
    if (out_submission_id != nullptr) {
        if (!IsValidGuestRange(reinterpret_cast<VAddr>(out_submission_id), sizeof(u32))) {
            return ORBIS_KERNEL_ERROR_EFAULT;
        }
        *out_submission_id = id;
    }
    return WriteAprResult(result);
}

s32 PS4_SYSV_ABI sceKernelAprWaitCommandBuffer(u32 submission_id, u64, void* result) {
    if (!Core::IsGlobalPs5RuntimeMode()) {
        return ORBIS_KERNEL_ERROR_ENOSYS;
    }
    {
        std::scoped_lock lock{g_apr_submission_mutex};
        if (g_apr_submissions.erase(submission_id) == 0) {
            LOG_WARNING(Kernel, "PS5 mode: APR wait for unknown submission id {:#x}; "
                                "treating as completed",
                        submission_id);
        }
    }
    return WriteAprResult(result);
}

s32 PS4_SYSV_ABI sceKernelWaitCommandBufferCompletion(u64 submission_id, u64 arg1, void* result,
                                                      u64 arg3, u64 arg4, u64 arg5) {
    if (!Core::IsGlobalPs5RuntimeMode()) {
        return ORBIS_KERNEL_ERROR_ENOSYS;
    }
    const auto write_result = WriteAprResult(result);
    return write_result;
}

s32 PS4_SYSV_ABI sceKernelAprSubmitCommandBuffer(void* command_buffer, u64) {
    if (!Core::IsGlobalPs5RuntimeMode() || command_buffer == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    AllocateAprSubmissionId(reinterpret_cast<VAddr>(command_buffer));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAprSubmitCommandBufferAndGetId(void* command_buffer, u64,
                                                         u32* out_submission_id) {
    if (!Core::IsGlobalPs5RuntimeMode() || command_buffer == nullptr ||
        out_submission_id == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (!IsValidGuestRange(reinterpret_cast<VAddr>(out_submission_id), sizeof(u32))) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    *out_submission_id = AllocateAprSubmissionId(reinterpret_cast<VAddr>(command_buffer));
    return ORBIS_OK;
}

void* PS4_SYSV_ABI sceKernelGetSanitizerNewReplaceExternal() {
    if (!Core::IsGlobalPs5RuntimeMode()) {
        return nullptr;
    }

    constexpr u64 TlsNewReplaceOffset = 0x300;
    constexpr u64 NewReplaceSize = 0x68;
    VAddr address = 0;
    if (auto* tcb = Core::GetTcbBase(); tcb != nullptr) {
        address = reinterpret_cast<VAddr>(tcb) + TlsNewReplaceOffset;
    }
    if (!IsValidGuestRange(address, NewReplaceSize)) {
        address = reinterpret_cast<VAddr>(g_sanitizer_new_replace_fallback.data());
    }

    std::memset(reinterpret_cast<void*>(address), 0, NewReplaceSize);
    *reinterpret_cast<u64*>(address) = NewReplaceSize;
    return reinterpret_cast<void*>(address);
}

// Nominally: long sysconf(int name);
u64 PS4_SYSV_ABI posix_sysconf(s32 name) {
    switch (name) {
    case 0:
        return 0x20000;
    case POSIX_SC_ARG_MAX:
        return 0x588bc000;
    case POSIX_SC_CHILD_MAX:
        return 0x64;
    case POSIX_SC_CLK_TCK:
        return 0x20;
    case POSIX_SC_NGROUPS_MAX:
        return 0x644;
    case POSIX_SC_OPEN_MAX:
        return -0x1;
    case POSIX_SC_JOB_CONTROL:
        return 0x6;
    case POSIX_SC_SAVED_IDS:
        return 0x1;
    case POSIX_SC_VERSION:
        return 0x1;
    case POSIX_SC_BC_BASE_MAX:
        return 0x31069;
    case POSIX_SC_BC_DIM_MAX:
        return -0x1;
    case POSIX_SC_BC_SCALE_MAX:
        return 0x31069;
    case POSIX_SC_BC_STRING_MAX:
        return 0x31069;
    case POSIX_SC_COLL_WEIGHTS_MAX:
        return -0x1;
    case POSIX_SC_EXPR_NEST_MAX:
        return -0x1;
    case POSIX_SC_LINE_MAX:
        return 0x31069;
    case POSIX_SC_RE_DUP_MAX:
        return 0x31069;
    case POSIX_SC_2_VERSION:
        return 0x31069;
    case POSIX_SC_2_C_BIND:
        return 0x31069;
    case POSIX_SC_2_C_DEV:
        return 0x31069;
    case POSIX_SC_2_CHAR_TERM:
        return 0x31069;
    case POSIX_SC_2_FORT_DEV:
        return 0x31069;
    case POSIX_SC_2_FORT_RUN:
        return 0x31069;
    case POSIX_SC_2_LOCALEDEF:
        return -0x1;
    case POSIX_SC_2_SW_DEV:
        return -0x1;
    case POSIX_SC_2_UPE:
        return 0x0;
    case POSIX_SC_STREAM_MAX:
        return 0x7fffffff;
    case POSIX_SC_TZNAME_MAX:
        return -0x1;
    case POSIX_SC_ASYNCHRONOUS_IO:
        return 0x8000;
    case POSIX_SC_MAPPED_FILES:
        return 0x31069;
    case POSIX_SC_MEMLOCK:
        return 0x4000;
    case POSIX_SC_MEMLOCK_RANGE:
        return 0x1e;
    case POSIX_SC_MEMORY_PROTECTION:
        return 0x100;
    case POSIX_SC_MESSAGE_PASSING:
        return 0x7fffffff;
    case POSIX_SC_PRIORITIZED_IO:
        return -0x1;
    case POSIX_SC_PRIORITY_SCHEDULING:
        return -0x1;
    case POSIX_SC_REALTIME_SIGNALS:
        return 0x63;
    case POSIX_SC_SEMAPHORES:
        return 0x800;
    case POSIX_SC_FSYNC:
        return 0x63;
    case POSIX_SC_SHARED_MEMORY_OBJECTS:
        return 0x3e8;
    case POSIX_SC_SYNCHRONIZED_IO:
        return 0x2;
    case POSIX_SC_THREAD_ATTR_STACKSIZE:
        return 0x1;
    case POSIX_SC_THREAD_CPUTIME:
        return 0x1;
    case POSIX_SC_THREAD_DESTRUCTOR_ITERATIONS:
        return 0x48000;
    case POSIX_SC_THREAD_KEYS_MAX:
        return 0x1a078630b2dd7;
    case POSIX_SC_THREAD_PRIO_INHERIT:
        return -0x1;
    case POSIX_SC_THREAD_PRIO_PROTECT:
        return -0x1;
    case POSIX_SC_THREAD_PRIORITY_SCHEDULING:
        return 0x2bc;
    case POSIX_SC_THREAD_PROCESS_SHARED:
        return 0x2bc;
    case POSIX_SC_THREAD_SAFE_FUNCTIONS:
        return 0x1;
    case POSIX_SC_THREAD_SPORADIC_SERVER:
        return -0x1;
    case POSIX_SC_THREAD_STACK_MIN:
        return 0x1;
    case POSIX_SC_THREAD_THREADS_MAX:
        return 0x1;
    case POSIX_SC_TIMEOUTS:
        return -0x1;
    // Manually specified
    case POSIX_SC_PAGESIZE:
        return posix_getpagesize();
    default:
        LOG_ERROR(Lib_Kernel, "unhandled {}", name);
        return 0;
    }
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    service_thread = std::jthread{KernelServiceThread};

    Libraries::Kernel::RegisterFileSystem(sym);
    Libraries::Kernel::RegisterTime(sym);
    Libraries::Kernel::RegisterThreads(sym);
    Libraries::Kernel::RegisterKernelEventFlag(sym);
    Libraries::Kernel::RegisterMemory(sym);
    Libraries::Kernel::RegisterEventQueue(sym);
    Libraries::Kernel::RegisterProcess(sym);
    Libraries::Kernel::RegisterException(sym);
    Libraries::Kernel::RegisterAio(sym);
    Libraries::Kernel::RegisterDebug(sym);

    LIB_OBJ("f7uOxY9mM1U", "libkernel", 1, "libkernel", &g_stack_chk_guard);
    LIB_FUNCTION("D4yla3vx4tY", "libkernel", 1, "libkernel", sceKernelError);
    LIB_FUNCTION("YeU23Szo3BM", "libkernel", 1, "libkernel", sceKernelGetAllowedSdkVersionOnSystem);
    LIB_FUNCTION("Mv1zUObHvXI", "libkernel", 1, "libkernel", sceKernelGetSystemSwVersion);
    LIB_FUNCTION("igMefp4SAv0", "libkernel", 1, "libkernel", get_authinfo);
    LIB_FUNCTION("G-MYv5erXaU", "libkernel", 1, "libkernel", sceKernelGetAppInfo);
    LIB_FUNCTION("PfccT7qURYE", "libkernel", 1, "libkernel", kernel_ioctl);
    LIB_FUNCTION("wW+k21cmbwQ", "libkernel", 1, "libkernel", kernel_ioctl);
    LIB_FUNCTION("JGfTMBOdUJo", "libkernel", 1, "libkernel", sceKernelGetFsSandboxRandomWord);
    LIB_FUNCTION("6xVpy0Fdq+I", "libkernel", 1, "libkernel", _sigprocmask);
    LIB_FUNCTION("Xjoosiw+XPI", "libkernel", 1, "libkernel", sceKernelUuidCreate);
    LIB_FUNCTION("Ou3iL1abvng", "libkernel", 1, "libkernel", stack_chk_fail);
    LIB_FUNCTION("9BcDykPmo1I", "libkernel", 1, "libkernel", __Error);
    LIB_FUNCTION("k+AXqu2-eBc", "libkernel", 1, "libkernel", posix_getpagesize);
    LIB_FUNCTION("k+AXqu2-eBc", "libScePosix", 1, "libkernel", posix_getpagesize);

    LIB_FUNCTION("mkawd0NA9ts", "libkernel", 1, "libkernel", posix_sysconf);
    LIB_FUNCTION("mkawd0NA9ts", "libScePosix", 1, "libkernel", posix_sysconf);

    LIB_FUNCTION("NWtTN10cJzE", "libSceLibcInternalExt", 1, "libSceLibcInternal",
                 sceLibcHeapGetTraceInfo);

    // network
    LIB_FUNCTION("XVL8So3QJUk", "libkernel", 1, "libkernel", Libraries::Net::sys_connect);
    LIB_FUNCTION("pG70GT5yRo4", "libkernel", 1, "libkernel", Libraries::Net::sys_socketex);
    LIB_FUNCTION("KuOmgKoqCdY", "libkernel", 1, "libkernel", Libraries::Net::sys_bind);
    LIB_FUNCTION("6O8EwYOgH9Y", "libkernel", 1, "libkernel", Libraries::Net::sys_getsockopt);
    LIB_FUNCTION("fFxGkxF2bVo", "libkernel", 1, "libkernel", Libraries::Net::sys_setsockopt);
    LIB_FUNCTION("pxnCmagrtao", "libkernel", 1, "libkernel", Libraries::Net::sys_listen);
    LIB_FUNCTION("3e+4Iv7IJ8U", "libkernel", 1, "libkernel", Libraries::Net::sys_accept);
    LIB_FUNCTION("TUuiYS2kE8s", "libkernel", 1, "libkernel", Libraries::Net::sys_shutdown);
    LIB_FUNCTION("TU-d9PfIHPM", "libkernel", 1, "libkernel", Libraries::Net::sys_socket);
    LIB_FUNCTION("MZb0GKT3mo8", "libkernel", 1, "libkernel", Libraries::Net::sys_socketpair);
    LIB_FUNCTION("MZb0GKT3mo8", "libkernel_ps2emu", 1, "libkernel", Libraries::Net::sys_socketpair);
    LIB_FUNCTION("K1S8oc61xiM", "libkernel", 1, "libkernel", Libraries::Net::sys_htonl);
    LIB_FUNCTION("jogUIsOV3-U", "libkernel", 1, "libkernel", Libraries::Net::sys_htons);
    LIB_FUNCTION("fZOeZIOEmLw", "libkernel", 1, "libkernel", Libraries::Net::sys_send);
    LIB_FUNCTION("oBr313PppNE", "libkernel", 1, "libkernel", Libraries::Net::sys_sendto);
    LIB_FUNCTION("Ez8xjo9UF4E", "libkernel", 1, "libkernel", Libraries::Net::sys_recv);
    LIB_FUNCTION("lUk6wrGXyMw", "libkernel", 1, "libkernel", Libraries::Net::sys_recvfrom);

    LIB_FUNCTION("TU-d9PfIHPM", "libScePosix", 1, "libkernel", Libraries::Net::sys_socket);
    LIB_FUNCTION("fZOeZIOEmLw", "libScePosix", 1, "libkernel", Libraries::Net::sys_send);
    LIB_FUNCTION("oBr313PppNE", "libScePosix", 1, "libkernel", Libraries::Net::sys_sendto);
    LIB_FUNCTION("Ez8xjo9UF4E", "libScePosix", 1, "libkernel", Libraries::Net::sys_recv);
    LIB_FUNCTION("lUk6wrGXyMw", "libScePosix", 1, "libkernel", Libraries::Net::sys_recvfrom);
    LIB_FUNCTION("hI7oVeOluPM", "libScePosix", 1, "libkernel", Libraries::Net::sys_recvmsg);
    LIB_FUNCTION("TXFFFiNldU8", "libScePosix", 1, "libkernel", Libraries::Net::sys_getpeername);
    LIB_FUNCTION("6O8EwYOgH9Y", "libScePosix", 1, "libkernel", Libraries::Net::sys_getsockopt);
    LIB_FUNCTION("fFxGkxF2bVo", "libScePosix", 1, "libkernel", Libraries::Net::sys_setsockopt);
    LIB_FUNCTION("RenI1lL1WFk", "libScePosix", 1, "libkernel", Libraries::Net::sys_getsockname);
    LIB_FUNCTION("KuOmgKoqCdY", "libScePosix", 1, "libkernel", Libraries::Net::sys_bind);
    LIB_FUNCTION("5jRCs2axtr4", "libScePosix", 1, "libkernel",
                 Libraries::Net::sceNetInetNtop); // TODO fix it to sys_ ...
    LIB_FUNCTION("4n51s0zEf0c", "libScePosix", 1, "libkernel",
                 Libraries::Net::sceNetInetPton); // TODO fix it to sys_ ...
    LIB_FUNCTION("XVL8So3QJUk", "libScePosix", 1, "libkernel", Libraries::Net::sys_connect);
    LIB_FUNCTION("3e+4Iv7IJ8U", "libScePosix", 1, "libkernel", Libraries::Net::sys_accept);
    LIB_FUNCTION("aNeavPDNKzA", "libScePosix", 1, "libkernel", Libraries::Net::sys_sendmsg);
    LIB_FUNCTION("pxnCmagrtao", "libScePosix", 1, "libkernel", Libraries::Net::sys_listen);

    LIB_FUNCTION("4oXYe9Xmk0Q", "libkernel", 1, "libkernel", sceKernelGetGPI);
    LIB_FUNCTION("ca7v6Cxulzs", "libkernel", 1, "libkernel", sceKernelSetGPO);
    LIB_FUNCTION("iKJMWrAumPE", "libkernel", 1, "libkernel", getargc);
    LIB_FUNCTION("FJmglmTMdr4", "libkernel", 1, "libkernel", getargv);

    if (Core::IsGlobalPs5RuntimeMode()) {
        LIB_FUNCTION("pB-yGZ2nQ9o", "libkernel", 1, "libkernel",
                     _sceKernelSetThreadAtexitCount);
        LIB_FUNCTION("WhCc1w3EhSI", "libkernel", 1, "libkernel",
                     _sceKernelSetThreadAtexitReport);
        LIB_FUNCTION("rNhWz+lvOMU", "libkernel", 1, "libkernel", _sceKernelSetThreadDtors);
        LIB_FUNCTION("Tz4RNUCBbGI", "libkernel", 1, "libkernel",
                     _sceKernelRtldThreadAtexitIncrement);
        LIB_FUNCTION("8OnWXlgQlvo", "libkernel", 1, "libkernel",
                     _sceKernelRtldThreadAtexitDecrement);
        LIB_FUNCTION("8zLSfEfW5AU", "libSceCoredump", 1, "libSceCoredump",
                     sceCoredumpRegisterCoredumpHandler);
        LIB_FUNCTION("ASoW5WE-UPo", "libkernel", 1, "libkernel",
                     sceKernelAprSubmitCommandBufferAndGetResult);
        LIB_FUNCTION("rqwFKI4PAiM", "libkernel", 1, "libkernel",
                     sceKernelAprWaitCommandBuffer);
        LIB_FUNCTION("eE4Szl8sil8", "libkernel", 1, "libkernel",
                     sceKernelAprSubmitCommandBuffer);
        LIB_FUNCTION("qvMUCyyaCSI", "libkernel", 1, "libkernel",
                     sceKernelAprSubmitCommandBufferAndGetId);
        LIB_FUNCTION("3GqBPApWgPY", "libkernel", 1, "libkernel",
                     sceKernelWaitCommandBufferCompletion);
        LIB_FUNCTION("bnZxYgAFeA0", "libkernel", 1, "libkernel",
                     sceKernelGetSanitizerNewReplaceExternal);
    }
}

} // namespace Libraries::Kernel
