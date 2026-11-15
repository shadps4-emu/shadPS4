// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include <boost/asio/io_context.hpp>

#include "common/assert.h"
#include "common/debug.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/polyfill_thread.h"
#include "common/thread.h"
#include "common/va_ctx.h"
#include "core/cpu_patches.h"
#include "core/file_sys/fs.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/coredump/coredump.h"
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
#include "core/libraries/net/net.h"
#include "core/libraries/net/net_kernel.h"

#ifdef _WIN64
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif
#include <common/singleton.h>
#include <core/linker.h>
#include "aio.h"

namespace Libraries::Kernel {

static u64 g_stack_chk_guard = 0xDEADBEEF54321ABC; // dummy return
char const* g_environment[64];
static const char* g_progname = "eboot.bin";

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
    void* caller = __builtin_return_address(0);
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    auto* mod = linker ? linker->FindByAddress(reinterpret_cast<VAddr>(caller)) : nullptr;
    if (mod) {
        LOG_CRITICAL(Lib_Kernel, "called from {:p} (module: {}, offset: {:#x})", caller, mod->name,
                     reinterpret_cast<VAddr>(caller) - mod->GetBaseAddress());
    } else {
        LOG_CRITICAL(Lib_Kernel, "called from {:p} (unknown module)", caller);
    }
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

s32 NativeToPosixErrno(s32 const e) {
    // Some error numbers are different between supported OSes
    switch (e) {
    case 0:
        return 0;
    case EPERM:
        return POSIX_EPERM;
        break;
    case ENOENT:
        return POSIX_ENOENT;
        break;
    case EINTR:
        return POSIX_EINTR;
        break;
    case EDEADLK:
        return POSIX_EDEADLK;
        break;
    case ENOMEM:
        return POSIX_ENOMEM;
        break;
    case EACCES:
        return POSIX_EACCES;
        break;
    case EFAULT:
        return POSIX_EFAULT;
        break;
    case EINVAL:
        return POSIX_EINVAL;
        break;
    case ENOSPC:
        return POSIX_ENOSPC;
        break;
    case ERANGE:
        return POSIX_ERANGE;
        break;
    case EAGAIN:
        return POSIX_EAGAIN;
        break;
    case ETIMEDOUT:
        return POSIX_ETIMEDOUT;
        break;
    default:
        LOG_WARNING(Kernel, "Unhandled errno {}", e);
        return e;
    }
}

void SetPosixErrno(s32 e) {
    g_posix_errno = NativeToPosixErrno(e);
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

s32 PS4_SYSV_ABI sceKernelTitleWorkaroundIsEnabled(OrbisKernelTitleWorkaround* tw, s32 bit,
                                                   s32* result) {
    LOG_ERROR(Lib_Kernel, "(STUBBED) called, bit {:#x}", bit);
    if (!tw || !result) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }

    // Maximum bit value is known to change with new firmwares.
    if (bit >= 0x3a) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    // Straight from decompilation, most uses rely on workaround data from sceKernelGetAppInfo.
    *result = ((tw->ids[bit >> 6] >> (bit & 0x3f)) & 1);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelGetProcessType(s32 pid) {
    LOG_DEBUG(Lib_Kernel, "called, pid: {}", pid);
    if (pid != GLOBAL_PID && pid != -1 && pid != 0) {
        return ORBIS_KERNEL_ERROR_ESRCH;
    }
    return ORBIS_KERNEL_PROCESS_TYPE_BIG_APP;
}

s32 PS4_SYSV_ABI __sys_regmgr_call(u32 op, u32 key, void* result, void* value, u64 len) {
    LOG_ERROR(Lib_Kernel, "(STUBBED) called, op: {:#x}, key: {}, len: {}", op, key, len);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelGetOpenPsId(void* psid) {
    LOG_TRACE(Lib_Kernel, "called, psid = {}", fmt::ptr(psid));
    if (!psid) {
        return POSIX_EINVAL;
    }
    static constexpr u8 default_psid[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                            0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
    std::memcpy(psid, default_psid, sizeof(default_psid));
    return ORBIS_OK;
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

static s32 CopyOutSysctl(void* oldp, size_t* oldlenp, const void* src, size_t src_len) {
    if (oldlenp == nullptr) {
        return 0;
    }
    if (oldp != nullptr) {
        const size_t copy_len = std::min(*oldlenp, src_len);
        std::memcpy(oldp, src, copy_len);
    }
    *oldlenp = src_len;
    return 0;
}

s32 PS4_SYSV_ABI posix_sysctlbyname(const char* name, void* oldp, size_t* oldlenp, const void* newp,
                                    size_t newlen) {
    if (name == nullptr) {
        SetPosixErrno(EFAULT);
        return -1;
    }

    const std::string_view name_view{name};

    if (name_view == "machdep.tsc_freq") {
        const u64 freq =
            Libraries::Kernel::sceKernelIsNeoMode() ? 2'133'000'000ULL : 1'594'000'000ULL;
        return CopyOutSysctl(oldp, oldlenp, &freq, sizeof(freq));
    }

    if (name_view == "dev.cpu.0.freq") {
        const s32 freq = Libraries::Kernel::sceKernelIsNeoMode() ? 2133 : 1594;
        return CopyOutSysctl(oldp, oldlenp, &freq, sizeof(freq));
    }

    if (name_view == "kern.proc.ptc") {
        const u64 ptc = Dev::GetInitialPtc();
        return CopyOutSysctl(oldp, oldlenp, &ptc, sizeof(ptc));
    }

    if (name_view == "kern.neomode") {
        const s32 neomode = Libraries::Kernel::sceKernelIsNeoMode();
        return CopyOutSysctl(oldp, oldlenp, &neomode, sizeof(neomode));
    }

    if (name_view == "kern.neomode_switch") {
        if (newp != nullptr) {
            Libraries::Kernel::sceKernelSwitchToNeoMode(0);
        }
        const s32 neomode = Libraries::Kernel::sceKernelIsNeoMode();
        return CopyOutSysctl(oldp, oldlenp, &neomode, sizeof(neomode));
    }

    if (name_view == "machdep.bootparams.base_ps4_mode") {
        const s32 base_ps4_mode = Libraries::Kernel::sceKernelIsNeoMode() ? 0 : 1;
        return CopyOutSysctl(oldp, oldlenp, &base_ps4_mode, sizeof(base_ps4_mode));
    }

    if (name_view == "hw.model") {
        static constexpr char model[] = "PlayStation 4";
        return CopyOutSysctl(oldp, oldlenp, model, sizeof(model));
    }

    if (name_view == "hw.ncpu") {
        static constexpr s32 ncpu = 8;
        return CopyOutSysctl(oldp, oldlenp, &ncpu, sizeof(ncpu));
    }

    if (name_view == "hw.pagesize") {
        static constexpr s32 pagesize = 16384;
        return CopyOutSysctl(oldp, oldlenp, &pagesize, sizeof(pagesize));
    }

    if (name_view == "hw.physmem") {
        static constexpr u64 physmem = 8ULL * 1024 * 1024 * 1024;
        return CopyOutSysctl(oldp, oldlenp, &physmem, sizeof(physmem));
    }

    if (name_view == "hw.usermem") {
        static constexpr u64 usermem = 5ULL * 1024 * 1024 * 1024;
        return CopyOutSysctl(oldp, oldlenp, &usermem, sizeof(usermem));
    }

    if (name_view == "hw.realmem") {
        static constexpr u64 realmem = 8ULL * 1024 * 1024 * 1024;
        return CopyOutSysctl(oldp, oldlenp, &realmem, sizeof(realmem));
    }

    if (name_view == "hw.byteorder") {
        static constexpr s32 byteorder = 1234;
        return CopyOutSysctl(oldp, oldlenp, &byteorder, sizeof(byteorder));
    }

    if (name_view == "hw.machine") {
        static constexpr char machine[] = "amd64";
        return CopyOutSysctl(oldp, oldlenp, machine, sizeof(machine));
    }

    if (name_view == "hw.machine_arch") {
        static constexpr char arch[] = "amd64";
        return CopyOutSysctl(oldp, oldlenp, arch, sizeof(arch));
    }

    if (name_view == "hw.floatingpoint") {
        static constexpr s32 floatingpt = 1;
        return CopyOutSysctl(oldp, oldlenp, &floatingpt, sizeof(floatingpt));
    }

    LOG_WARNING(Lib_Kernel, "sysctlbyname: unknown name '{}'", name);
    SetPosixErrno(ENOENT);
    return -1;
}

namespace {
// FreeBSD standard top-level MIB identifiers
constexpr s32 CTL_KERN = 1;
constexpr s32 CTL_HW = 6;
constexpr s32 CTL_MACHDEP = 7;

// CTL_HW identifiers
constexpr s32 HW_MACHINE = 1;
constexpr s32 HW_MODEL = 2;
constexpr s32 HW_NCPU = 3;
constexpr s32 HW_BYTEORDER = 4;
constexpr s32 HW_PHYSMEM = 5;
constexpr s32 HW_USERMEM = 6;
constexpr s32 HW_PAGESIZE = 7;
constexpr s32 HW_FLOATINGPT = 9;
constexpr s32 HW_MACHINE_ARCH = 10;
constexpr s32 HW_REALMEM = 11;

// CTL_MACHDEP identifiers
constexpr s32 CPU_TSC_FREQ = 1;
constexpr s32 CPU_FREQ = 2;
constexpr s32 BOOTPARAMS_BASE_PS4_MODE = 3;

// CTL_KERN identifiers
constexpr s32 KERN_NEOMODE = 100;
constexpr s32 KERN_NEOMODE_SWITCH = 101;
constexpr s32 KERN_PROC_PTC = 102;
} // namespace

s32 PS4_SYSV_ABI posix_sysctlnametomib(const char* name, s32* mibp, size_t* sizep) {
    if (name == nullptr || mibp == nullptr || sizep == nullptr) {
        SetPosixErrno(EFAULT);
        return -1;
    }
    if (*sizep < 2) {
        SetPosixErrno(ENOMEM);
        return -1;
    }

    const std::string_view s{name};
    if (s == "machdep.tsc_freq") {
        mibp[0] = CTL_MACHDEP;
        mibp[1] = CPU_TSC_FREQ;
    } else if (s == "dev.cpu.0.freq") {
        mibp[0] = CTL_MACHDEP;
        mibp[1] = CPU_FREQ;
    } else if (s == "machdep.bootparams.base_ps4_mode") {
        mibp[0] = CTL_MACHDEP;
        mibp[1] = BOOTPARAMS_BASE_PS4_MODE;
    } else if (s == "kern.neomode") {
        mibp[0] = CTL_KERN;
        mibp[1] = KERN_NEOMODE;
    } else if (s == "kern.neomode_switch") {
        mibp[0] = CTL_KERN;
        mibp[1] = KERN_NEOMODE_SWITCH;
    } else if (s == "kern.proc.ptc") {
        mibp[0] = CTL_KERN;
        mibp[1] = KERN_PROC_PTC;
    } else if (s == "hw.model") {
        mibp[0] = CTL_HW;
        mibp[1] = HW_MODEL;
    } else if (s == "hw.ncpu") {
        mibp[0] = CTL_HW;
        mibp[1] = HW_NCPU;
    } else if (s == "hw.pagesize") {
        mibp[0] = CTL_HW;
        mibp[1] = HW_PAGESIZE;
    } else if (s == "hw.physmem") {
        mibp[0] = CTL_HW;
        mibp[1] = HW_PHYSMEM;
    } else if (s == "hw.usermem") {
        mibp[0] = CTL_HW;
        mibp[1] = HW_USERMEM;
    } else if (s == "hw.realmem") {
        mibp[0] = CTL_HW;
        mibp[1] = HW_REALMEM;
    } else if (s == "hw.byteorder") {
        mibp[0] = CTL_HW;
        mibp[1] = HW_BYTEORDER;
    } else if (s == "hw.machine") {
        mibp[0] = CTL_HW;
        mibp[1] = HW_MACHINE;
    } else if (s == "hw.machine_arch") {
        mibp[0] = CTL_HW;
        mibp[1] = HW_MACHINE_ARCH;
    } else if (s == "hw.floatingpoint") {
        mibp[0] = CTL_HW;
        mibp[1] = HW_FLOATINGPT;
    } else {
        LOG_WARNING(Lib_Kernel, "sysctlnametomib: unknown name '{}'", name);
        SetPosixErrno(ENOENT);
        return -1;
    }

    *sizep = 2;
    return 0;
}

s32 PS4_SYSV_ABI posix_sysctl(const s32* name, u32 namelen, void* oldp, size_t* oldlenp,
                              const void* newp, size_t newlen) {
    if (name == nullptr || namelen < 2) {
        SetPosixErrno(EINVAL);
        return -1;
    }

    if (name[0] == CTL_HW) {
        switch (name[1]) {
        case HW_MACHINE: {
            static constexpr char machine[] = "amd64";
            return CopyOutSysctl(oldp, oldlenp, machine, sizeof(machine));
        }
        case HW_MODEL: {
            static constexpr char model[] = "PlayStation 4";
            return CopyOutSysctl(oldp, oldlenp, model, sizeof(model));
        }
        case HW_NCPU: {
            static constexpr s32 ncpu = 8;
            return CopyOutSysctl(oldp, oldlenp, &ncpu, sizeof(ncpu));
        }
        case HW_BYTEORDER: {
            static constexpr s32 byteorder = 1234;
            return CopyOutSysctl(oldp, oldlenp, &byteorder, sizeof(byteorder));
        }
        case HW_PHYSMEM: {
            static constexpr u64 physmem = 8ULL * 1024 * 1024 * 1024;
            return CopyOutSysctl(oldp, oldlenp, &physmem, sizeof(physmem));
        }
        case HW_USERMEM: {
            static constexpr u64 usermem = 5ULL * 1024 * 1024 * 1024;
            return CopyOutSysctl(oldp, oldlenp, &usermem, sizeof(usermem));
        }
        case HW_PAGESIZE: {
            static constexpr s32 pagesize = 16384;
            return CopyOutSysctl(oldp, oldlenp, &pagesize, sizeof(pagesize));
        }
        case HW_FLOATINGPT: {
            static constexpr s32 floatingpt = 1;
            return CopyOutSysctl(oldp, oldlenp, &floatingpt, sizeof(floatingpt));
        }
        case HW_MACHINE_ARCH: {
            static constexpr char arch[] = "amd64";
            return CopyOutSysctl(oldp, oldlenp, arch, sizeof(arch));
        }
        case HW_REALMEM: {
            static constexpr u64 realmem = 8ULL * 1024 * 1024 * 1024;
            return CopyOutSysctl(oldp, oldlenp, &realmem, sizeof(realmem));
        }
        default:
            LOG_WARNING(Lib_Kernel, "sysctl: unhandled CTL_HW mib {}", name[1]);
            SetPosixErrno(ENOENT);
            return -1;
        }
    }

    if (name[0] == CTL_MACHDEP) {
        switch (name[1]) {
        case CPU_TSC_FREQ: {
            const u64 freq =
                Libraries::Kernel::sceKernelIsNeoMode() ? 2'133'000'000ULL : 1'594'000'000ULL;
            return CopyOutSysctl(oldp, oldlenp, &freq, sizeof(freq));
        }
        case CPU_FREQ: {
            const s32 freq = Libraries::Kernel::sceKernelIsNeoMode() ? 2133 : 1594;
            return CopyOutSysctl(oldp, oldlenp, &freq, sizeof(freq));
        }
        case BOOTPARAMS_BASE_PS4_MODE: {
            const s32 base_ps4_mode = Libraries::Kernel::sceKernelIsNeoMode() ? 0 : 1;
            return CopyOutSysctl(oldp, oldlenp, &base_ps4_mode, sizeof(base_ps4_mode));
        }
        default:
            LOG_WARNING(Lib_Kernel, "sysctl: unhandled CTL_MACHDEP mib {}", name[1]);
            SetPosixErrno(ENOENT);
            return -1;
        }
    }

    if (name[0] == CTL_KERN) {
        switch (name[1]) {
        case KERN_NEOMODE: {
            const s32 neomode = Libraries::Kernel::sceKernelIsNeoMode();
            return CopyOutSysctl(oldp, oldlenp, &neomode, sizeof(neomode));
        }
        case KERN_NEOMODE_SWITCH: {
            if (newp != nullptr) {
                Libraries::Kernel::sceKernelSwitchToNeoMode(0);
            }
            const s32 neomode = Libraries::Kernel::sceKernelIsNeoMode();
            return CopyOutSysctl(oldp, oldlenp, &neomode, sizeof(neomode));
        }
        case KERN_PROC_PTC: {
            const u64 ptc = Dev::GetInitialPtc();
            return CopyOutSysctl(oldp, oldlenp, &ptc, sizeof(ptc));
        }
        default:
            LOG_WARNING(Lib_Kernel, "sysctl: unhandled CTL_KERN mib {}", name[1]);
            SetPosixErrno(ENOENT);
            return -1;
        }
    }

    LOG_WARNING(Lib_Kernel, "sysctl: unhandled MIB name[0]={}, name[1]={}", name[0], name[1]);
    SetPosixErrno(ENOENT);
    return -1;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    service_thread = std::jthread{KernelServiceThread};

    static char const** kernel_environ = g_environment;

    // Before any socket exists: net ids are file descriptors from the kernel's table.
    Libraries::Net::InstallKernelIntegration();

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
    Libraries::Kernel::RegisterCoredump(sym);

    LIB_OBJ("f7uOxY9mM1U", "libkernel", 1, "libkernel", &g_stack_chk_guard);
    LIB_OBJ("+2thxYZ4syk", "libkernel", 1, "libkernel", &kernel_environ);
    LIB_OBJ("djxxOmW6-aw", "libkernel", 1, "libkernel", &g_progname);
    LIB_FUNCTION("D4yla3vx4tY", "libkernel", 1, "libkernel", sceKernelError);
    LIB_FUNCTION("YeU23Szo3BM", "libkernel", 1, "libkernel", sceKernelGetAllowedSdkVersionOnSystem);
    LIB_FUNCTION("Mv1zUObHvXI", "libkernel", 1, "libkernel", sceKernelGetSystemSwVersion);
    LIB_FUNCTION("igMefp4SAv0", "libkernel", 1, "libkernel", get_authinfo);
    LIB_FUNCTION("G-MYv5erXaU", "libkernel", 1, "libkernel", sceKernelGetAppInfo);
    LIB_FUNCTION("1yca4VvfcNA", "libkernel", 1, "libkernel", sceKernelTitleWorkaroundIsEnabled);
    LIB_FUNCTION("+g+UP8Pyfmo", "libkernel", 1, "libkernel", sceKernelGetProcessType);
    LIB_FUNCTION("PfccT7qURYE", "libkernel", 1, "libkernel", kernel_ioctl);
    LIB_FUNCTION("wW+k21cmbwQ", "libkernel", 1, "libkernel", kernel_ioctl);
    LIB_FUNCTION("JGfTMBOdUJo", "libkernel", 1, "libkernel", sceKernelGetFsSandboxRandomWord);
    LIB_FUNCTION("JGfTMBOdUJo", "libkernel_psmkit", 1, "libkernel",
                 sceKernelGetFsSandboxRandomWord);
    LIB_FUNCTION("6xVpy0Fdq+I", "libkernel", 1, "libkernel", _sigprocmask);
    LIB_FUNCTION("Xjoosiw+XPI", "libkernel", 1, "libkernel", sceKernelUuidCreate);
    LIB_FUNCTION("Ou3iL1abvng", "libkernel", 1, "libkernel", stack_chk_fail);
    LIB_FUNCTION("9BcDykPmo1I", "libkernel", 1, "libkernel", __Error);
    LIB_FUNCTION("k+AXqu2-eBc", "libkernel", 1, "libkernel", posix_getpagesize);
    LIB_FUNCTION("k+AXqu2-eBc", "libScePosix", 1, "libkernel", posix_getpagesize);
    LIB_FUNCTION("7NwggrWJ5cA", "libkernel", 1, "libkernel", __sys_regmgr_call);

    LIB_FUNCTION("mkawd0NA9ts", "libkernel", 1, "libkernel", posix_sysconf);
    LIB_FUNCTION("mkawd0NA9ts", "libScePosix", 1, "libkernel", posix_sysconf);

    LIB_FUNCTION("MhC53TKmjVA", "libkernel", 1, "libkernel", posix_sysctlbyname);
    LIB_FUNCTION("lZG-WlKxRqQ", "libScePosix", 1, "libkernel", posix_sysctlbyname);
    LIB_FUNCTION("DFmMT80xcNI", "libkernel", 1, "libkernel", posix_sysctl);
    LIB_FUNCTION("ul57hvm6mBc", "libScePosix", 1, "libkernel", posix_sysctl);
    LIB_FUNCTION("UkEV4hvMo1E", "libkernel", 1, "libkernel", posix_sysctlnametomib);

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

    LIB_FUNCTION("TUuiYS2kE8s", "libScePosix", 1, "libkernel", Libraries::Net::sys_shutdown);
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
    LIB_FUNCTION("DLORcroUqbc", "libkernel", 1, "libkernel", sceKernelGetOpenPsId);
    LIB_FUNCTION("DLORcroUqbc", "libSceOpenPsId", 1, "libkernel", sceKernelGetOpenPsId);
}

} // namespace Libraries::Kernel
