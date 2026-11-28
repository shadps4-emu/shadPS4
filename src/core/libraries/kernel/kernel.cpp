// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <string_view>
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
#include "core/emulator_settings.h"
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
#include "core/libraries/system/notification_ui.h"
#include "core/user_settings.h"

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

static std::string_view LookupSystemMessage(s32 msg_id, s32 lang) {
    switch (msg_id) {
    case 0x004:
        switch (lang) {
        case 0:
            return "%1のバッテリー残量が少なくなりました。";
        case 2:
            return "Le niveau de la batterie de %1 est faible.";
        case 3:
            return "Nivel de batería de %1 bajo.";
        case 4:
            return "Batterie von %1 ist fast leer.";
        case 5:
            return "La batteria di %1 si sta scaricando.";
        case 6:
            return "Het batterijniveau van %1 is laag.";
        case 7:
            return "Nível da bateria do %1 baixo.";
        case 8:
            return "Аккумулятор %1 разряжен.";
        case 9:
            return "%1의 배터리 전력이 부족합니다.";
        case 10:
            return "%1的剩餘充電量已減少。";
        case 11:
            return "%1的电池余量低。";
        case 12:
            return "Akun varaus laitteessa %1 on alhainen.";
        case 13:
            return "Batteriet i %1 börjar ta slut.";
        case 14:
            return "Batteriniveau på %1 er lavt.";
        case 15:
            return "Batterinivået på %1 er lavt.";
        case 16:
            return "Poziom baterii urządzenia %1 jest niski.";
        case 17:
            return "Nível da bateria do %1 baixo.";
        case 19:
            return "%1 pil seviyesi düşük.";
        case 20:
            return "Nivel de batería de %1 bajo.";
        case 21:
            return "مستوى البطارية ‎%1 منخفض.";
        case 22:
            return "Le niveau de la batterie de %1 est faible.";
        case 23:
            return "Stav nabití baterie %1 je nízký.";
        case 24:
            return "%1 akkumulátorszintje alacsony.";
        case 25:
            return "Το επίπεδο μπαταρίας της συσκευής %1 είναι χαμηλό.";
        case 26:
            return "Nivelul bateriei %1 este redus.";
        case 27:
            return "ระดับแบตเตอรี่ของ %1 ต่ำ";
        case 28:
            return "Mức pin %1 thấp.";
        case 29:
            return "Level baterai %1 rendah.";
        case 30:
            return "Батарея %1 розряджена.";
        default:
            return "%1 battery level low.";
        }
    case 0x005:
        switch (lang) {
        case 0:
            return "%1が接続解除されました。";
        case 2:
            return "%1 déconnecté.";
        case 3:
            return "%1 desconectado.";
        case 4:
            return "Verbindung zu %1 getrennt.";
        case 5:
            return "%1 disconnesso.";
        case 6:
            return "De verbinding met %1 is verbroken.";
        case 7:
            return "Dispositivo %1 desligado.";
        case 8:
            return "Устройство %1 отключено.";
        case 9:
            return "%1(이)가 접속 해제되었습니다.";
        case 10:
            return "%1已中斷連線。";
        case 11:
            return "%1已断开连接。";
        case 12:
            return "Yhteys laitteeseen %1 on katkaistu.";
        case 13:
            return "%1 har kopplats från.";
        case 14:
            return "%1 blev afbrudt.";
        case 15:
            return "%1 er frakoblet.";
        case 16:
            return "Odłączono %1.";
        case 17:
            return "Dispositivo %1 desconectado.";
        case 19:
            return "%1 bağlantısı kesildi.";
        case 20:
            return "%1 desconectado.";
        case 21:
            return "تم قطع اتصال ‎%1‎‏.";
        case 22:
            return "%1 déconnecté.";
        case 23:
            return "Zařízení %1 odpojeno.";
        case 24:
            return "%1 lekapcsolódva.";
        case 25:
            return "Η συσκευή %1 αποσυνδέθηκε.";
        case 26:
            return "%1 deconectat.";
        case 27:
            return "เลิกเชื่อมต่อ %1 แล้ว";
        case 28:
            return "%1 đã bị ngắt kết nối.";
        case 29:
            return "%1 terputus sambungannya.";
        case 30:
            return "%1 від’єднано.";
        default:
            return "%1 disconnected.";
        }
    case 0x0d2:
        switch (lang) {
        case 0:
            return "%1が接続されました。";
        case 2:
            return "%1 connecté.";
        case 3:
            return "%1 conectado.";
        case 4:
            return "%1 verbunden.";
        case 5:
            return "%1 connesso.";
        case 6:
            return "%1 is verbonden.";
        case 7:
            return "Dispositivo %1 ligado.";
        case 8:
            return "Устройство %1 подключено.";
        case 9:
            return "%1(이)가 접속되었습니다.";
        case 10:
            return "%1已連接。";
        case 11:
            return "%1已连接。";
        case 12:
            return "%1 on yhdistetty.";
        case 13:
            return "%1 har anslutits.";
        case 14:
            return "%1 tilsluttet.";
        case 15:
            return "%1 er tilkoblet.";
        case 16:
            return "Podłączono: %1.";
        case 17:
            return "Dispositivo %1 conectado.";
        case 19:
            return "%1 bağlı.";
        case 20:
            return "%1 conectado.";
        case 21:
            return "‎%1 متصل.";
        case 22:
            return "%1 connecté.";
        case 23:
            return "Zařízení %1 připojeno.";
        case 24:
            return "%1 kapcsolódva.";
        case 25:
            return "Η συσκευή %1 συνδέθηκε.";
        case 26:
            return "%1 conectat.";
        case 27:
            return "เชื่อมต่อ %1 แล้ว";
        case 28:
            return "%1 đã được kết nối.";
        case 29:
            return "%1 tersambung.";
        case 30:
            return "%1 підключено.";
        default:
            return "%1 connected.";
        }
    case 0x04b:
    case 0x04d:
        switch (lang) {
        case 0:
            return "%1は接続解除されました。バッテリーを充電してください。";
        case 2:
            return "%1 déconnecté. Rechargez la batterie.";
        case 3:
            return "%1 desconectado. Carga la batería.";
        case 4:
            return "Verbindung zu %1 getrennt. Lade den Akku auf.";
        case 5:
            return "%1 disconnesso. Carica la batteria.";
        case 6:
            return "De verbinding met %1 is verbroken. Laad de batterij op.";
        case 7:
            return "Dispositivo %1 desligado. Carregue a bateria.";
        case 8:
            return "Устройство %1 отключено. Зарядите аккумулятор.";
        case 9:
            return "%1(이)가 접속 해제되었습니다. 배터리를 충전해 주세요.";
        case 10:
            return "%1已中斷連線。請替電池充電。";
        case 11:
            return "%1已断开连接。请为电池充电。";
        case 12:
            return "Yhteys laitteeseen %1 on katkaistu. Lataa akku.";
        case 13:
            return "%1 har kopplats från. Ladda batteriet.";
        case 14:
            return "%1 blev afbrudt. Oplad batteriet.";
        case 15:
            return "%1 er frakoblet. Lad batteriet.";
        case 16:
            return "Odłączono %1. Naładuj baterię.";
        case 17:
            return "Dispositivo %1 desconectado. Carregue a bateria.";
        case 19:
            return "%1 bağlantısı kesildi. Pili şarj edin.";
        case 20:
            return "%1 desconectado. Carga la batería.";
        case 21:
            return "تم قطع اتصال ‎%1. اشحن البطارية.";
        case 22:
            return "%1 déconnecté. Rechargez la batterie.";
        case 23:
            return "Zařízení %1 odpojeno. Nabijte baterii.";
        case 24:
            return "%1 lekapcsolódva. Töltsd az akkumulátort.";
        case 25:
            return "H συσκευή %1 αποσυνδέθηκε. Φορτίστε την μπαταρία.";
        case 26:
            return "%1 deconectat. Încarcă bateria.";
        case 27:
            return "เลิกเชื่อมต่อ %1 แล้ว ชาร์จแบตเตอรี่";
        case 28:
            return "%1 đã bị ngắt kết nối. Sạc pin.";
        case 29:
            return "%1 terputus sambungannya. Isi daya baterai.";
        case 30:
            return "%1 від’єднано. Зарядіть акумулятор.";
        default:
            return "%1 disconnected. Charge the battery.";
        }
    case 0x166:
        switch (lang) {
        case 0:
            return "お使いのコントローラーは正規品ではない可能性があり、適切に動作しない可能性があ"
                   "ります。";
        case 2:
            return "Votre manette n'est peut-être pas une manette officielle et risque de ne pas "
                   "fonctionner correctement.";
        case 3:
            return "Es posible que tu mando no sea un mando oficial y no funcione de forma "
                   "correcta.";
        case 4:
            return "Dein Controller ist eventuell kein offiziell unterstützter Controller und "
                   "funktioniert möglicherweise nicht ordnungsgemäß.";
        case 5:
            return "Il tuo controller potrebbe non essere un controller ufficiale e non funzionare "
                   "correttamente.";
        case 6:
            return "Je controller is mogelijk geen officiële controller. Het kan zijn dat deze "
                   "hierdoor niet goed functioneert.";
        case 7:
            return "O seu comando poderá ser um comando não oficial, pelo que poderá não funcionar "
                   "corretamente.";
        case 8:
            return "Возможно, ваш контроллер — не официальный и может работать неправильно.";
        case 9:
            return "사용 중인 컨트롤러가 공식 컨트롤러가 아니기 때문에 제대로 작동하지 않을 수도 "
                   "있습니다.";
        case 10:
            return "您的控制器可能不是官方的控制器，所以也許會無法正常運作。";
        case 11:
            return "您的控制器可能不是官方控制器，因此可能无法正常工作。";
        case 12:
            return "Ohjaimesi ei ehkä ole virallinen ohjain eikä välttämättä toimi oikein.";
        case 13:
            return "Din handkontroll kanske inte är en officiell handkontroll och kanske inte "
                   "fungerar som den ska.";
        case 14:
            return "Din controller er muligvis ikke en officiel controller, og fungerer måske ikke "
                   "korrekt.";
        case 15:
            return "Kontrolleren din er kanskje ikke en offisiell kontroller, eller den fungerer "
                   "ikke som den skal.";
        case 16:
            return "Wykryty kontroler może nie być oficjalnym kontrolerem i może nie działać "
                   "prawidłowo.";
        case 17:
            return "É possível que seu controle não seja oficial e não funcione corretamente.";
        case 19:
            return "Kontrol cihazınız resmi kontrol cihazı olmayabilir ve düzgün çalışmayabilir.";
        case 20:
            return "Es posible que tu control no sea oficial y quizá no funcione correctamente.";
        case 21:
            return "ربما تكون وحدة التحكم التي تستخدمها غير معتمدة وقد لا تعمل بشكل سليم.";
        case 22:
            return "Il se peut que votre manette de jeu ne soit pas une manette de jeu officielle. "
                   "Par conséquent, elle pourrait ne pas fonctionner correctement.";
        case 23:
            return "Váš ovladač pravděpodobně není oficiální ovladač a nemusí fungovat řádně.";
        case 24:
            return "Előfordulhat, hogy nem hivatalos vezérlőt használsz, és a vezérlő nem működik "
                   "megfelelően.";
        case 25:
            return "Το χειριστήριό σας ενδέχεται να μην είναι επίσημο χειριστήριο και ίσως να μην "
                   "λειτουργεί σωστά.";
        case 26:
            return "Controlerul tău s-ar putea să nu fie unul oficial și să nu funcționeze "
                   "corespunzător.";
        case 27:
            return "คอนโทรลเลอร์ของคุณอาจจะไม่ใช่คอนโทรลเลอร์อย่างเป็นทางการและอาจไม่ทำงานอย่างเหมาะสม";
        case 28:
            return "Bộ điều khiển của bạn có thể không phải là bộ điều khiển chính thức và không "
                   "hoạt động đúng chức năng.";
        case 29:
            return "Controller Anda mungkin bukan controller resmi dan tidak berfungsi dengan "
                   "benar.";
        case 30:
            return "Можливо, ваш контролер — не офіційний та може працювати неправильно.";
        default:
            return "Your controller might not be an official controller and may not function "
                   "properly.";
        }
    case 0x062:
        switch (lang) {
        case 0:
            return "スクリーンショットを保存しました。";
        case 2:
            return "Capture d'écran sauvegardée.";
        case 3:
            return "Captura de pantalla guardada.";
        case 4:
            return "Screenshot gespeichert.";
        case 5:
            return "Istantanea della schermata salvata.";
        case 6:
            return "Screenshot opgeslagen.";
        case 7:
            return "Captura de ecrã guardada.";
        case 8:
            return "Снимок экрана сохранен.";
        case 9:
            return "스크린샷을 저장했습니다.";
        case 10:
            return "已保存螢幕截圖。";
        case 11:
            return "已保存画面截图。";
        case 12:
            return "Kuvakaappaus on tallennettu.";
        case 13:
            return "Skärmbilden har sparats.";
        case 14:
            return "Skærmbillede er gemt.";
        case 15:
            return "Skjermbilde er lagret.";
        case 16:
            return "Zrzut ekranu został zapisany.";
        case 17:
            return "Captura de tela salva.";
        case 19:
            return "Ekran görüntüsü kaydedildi.";
        case 20:
            return "Captura de pantalla guardada.";
        case 21:
            return "تم حفظ لقطة الشاشة.";
        case 22:
            return "Capture d'écran sauvegardée.";
        case 23:
            return "Snímek obrazovky byl uložen.";
        case 24:
            return "Mentett képernyő.";
        case 25:
            return "Το στιγμιότυπο αποθηκεύτηκε.";
        case 26:
            return "Captură de ecran salvată.";
        case 27:
            return "ภาพหน้าจอที่บันทึกไว้";
        case 28:
            return "Đã lưu ảnh chụp màn hình.";
        case 29:
            return "Cuplikan layar disimpan.";
        case 30:
            return "Знімок екрана збережено.";
        default:
            return "Screenshot saved.";
        }
    case 0x030:
        switch (lang) {
        case 0:
            return "現在、スクリーンショットを撮れません。";
        case 2:
            return "Capture d'écran impossible actuellement.";
        case 3:
            return "En este momento no se puede hacer una captura de pantalla.";
        case 4:
            return "Du kannst jetzt keinen Screenshot aufnehmen.";
        case 5:
            return "Impossibile scattare istantanea della schermata ora.";
        case 6:
            return "Kan momenteel geen screenshot maken.";
        case 7:
            return "Não é possível efetuar uma captura de ecrã agora.";
        case 8:
            return "Сейчас невозможно сделать снимок экрана.";
        case 9:
            return "현재, 스크린샷을 찍을 수 없습니다.";
        case 10:
            return "目前無法拍攝螢幕截圖。";
        case 11:
            return "目前无法拍摄画面截图。";
        case 12:
            return "Tästä ei voida ottaa kuvakaappausta.";
        case 13:
            return "Det går inte att ta en skärmbild nu.";
        case 14:
            return "Kan ikke tage skærmbillede nu.";
        case 15:
            return "Kan ikke ta et skjermbilde nå.";
        case 16:
            return "Nie można teraz robić zrzutów ekranu.";
        case 17:
            return "Não é possível fazer a captura de tela no momento.";
        case 19:
            return "Şu an ekran görüntüsü alınamıyor.";
        case 20:
            return "En este momento no se puede hacer una captura de pantalla.";
        case 21:
            return "تعذر أخذ لقطة شاشة الآن.";
        case 22:
            return "Capture d'écran impossible actuellement.";
        case 23:
            return "Nyní nelze pořídit snímek obrazovky.";
        case 24:
            return "Most nem készíthető képernyőkép.";
        case 25:
            return "Δεν είναι δυνατή η λήψη στιγμιότυπου αυτή τη στιγμή.";
        case 26:
            return "Nu se poate salva o captură de ecran acum.";
        case 27:
            return "ไม่สามารถถ่ายภาพหน้าจอได้ตอนนี้";
        case 28:
            return "Hiện không thể chụp ảnh chụp màn hình.";
        case 29:
            return "Tidak dapat mengambil cuplikan layar sekarang.";
        case 30:
            return "Зараз неможливо зробити знімок екрана.";
        default:
            return "Cannot take screenshot now.";
        }
    case 0x122:
        switch (lang) {
        case 0:
            return "[ダウンロードリスト]に追加されました。";
        case 2:
            return "Ajouté à [Téléchargements].";
        case 3:
            return "Añadido a [Descargas].";
        case 4:
            return "Zu [Downloads] hinzugefügt.";
        case 5:
            return "Aggiunto a [Download].";
        case 6:
            return "Toegevoegd aan [Downloads].";
        case 7:
            return "Adição efetuada a [Transferências].";
        case 8:
            return "Добавлено в [Загрузки].";
        case 9:
            return "[다운로드 리스트]에 추가되었습니다.";
        case 10:
            return "已追加至[下載內容]。";
        case 11:
            return "已添加至[下载内容]。";
        case 12:
            return "Lisätty kohteeseen [Lataukset].";
        case 13:
            return "Tillagt i [Nedladdningar].";
        case 14:
            return "Tilføjet til [Downloads].";
        case 15:
            return "Lagt til [Nedlastinger].";
        case 16:
            return "Dodano do [Pobrane].";
        case 17:
            return "Adicionado a [Downloads].";
        case 19:
            return "[İndirilenler] seçeneğine eklendi.";
        case 20:
            return "Agregado a [Descargas].";
        case 21:
            return "تمت الإضافة إلى [التنزيلات]‏.";
        case 22:
            return "Ajouté à [Téléchargements].";
        case 23:
            return "Přidáno do [Stažené soubory].";
        case 24:
            return "Hozzáadva [Letöltések] menühöz.";
        case 25:
            return "Προστέθηκε στο [Λήψεις].";
        case 26:
            return "Adăugat la [Descărcări].";
        case 27:
            return "เพิ่มไปยัง [การดาวน์โหลด] แล้ว";
        case 28:
            return "Đã thêm vào [Phần tải về].";
        case 29:
            return "Ditambahkan ke [Unduhan].";
        case 30:
            return "Додано до [Завантаження].";
        default:
            return "Added to [Downloads].";
        }
    case 0x124:
        switch (lang) {
        case 0:
            return "ダウンロードを再開しました。";
        case 2:
            return "Le téléchargement a repris.";
        case 3:
            return "Descarga reanudada.";
        case 4:
            return "Download fortgesetzt.";
        case 5:
            return "Download ripreso.";
        case 6:
            return "Downloaden hervat.";
        case 7:
            return "Transferência retomada.";
        case 8:
            return "Загрузка из сети возобновлена.";
        case 9:
            return "다운로드를 재개했습니다.";
        case 10:
            return "已繼續進行下載。";
        case 11:
            return "已继续下载。";
        case 12:
            return "Lataus jatkuu.";
        case 13:
            return "Nedladdningen har återupptagits.";
        case 14:
            return "Download genoptaget.";
        case 15:
            return "Nedlasting ble gjenopptatt.";
        case 16:
            return "Pobieranie wznowione.";
        case 17:
            return "Download retomado.";
        case 19:
            return "İndirme sürdürülüyor.";
        case 20:
            return "Descarga reanudada.";
        case 21:
            return "تم استئناف التنزيل.";
        case 22:
            return "Le téléchargement a repris.";
        case 23:
            return "Stahování pokračuje.";
        case 24:
            return "Letöltés folytatva.";
        case 25:
            return "Η λήψη συνεχίστηκε.";
        case 26:
            return "Descărcare reluată.";
        case 27:
            return "เริ่มดาวน์โหลดต่อ";
        case 28:
            return "Đã tiếp tục tải về.";
        case 29:
            return "Pengunduhan dilanjutkan.";
        case 30:
            return "Завантаження продовжено.";
        default:
            return "Download resumed.";
        }
    case 0x125:
        switch (lang) {
        case 0:
            return "ダウンロードできませんでした。";
        case 2:
            return "Téléchargement impossible.";
        case 3:
            return "No se ha podido descargar.";
        case 4:
            return "Herunterladen nicht möglich.";
        case 5:
            return "Impossibile scaricare.";
        case 6:
            return "Kan niet downloaden.";
        case 7:
            return "Não é possível efetuar transferência.";
        case 8:
            return "Нельзя выполнить загрузку.";
        case 9:
            return "다운로드하지 못했습니다.";
        case 10:
            return "無法下載。";
        case 11:
            return "无法下载。";
        case 12:
            return "Ei voida ladata.";
        case 13:
            return "Det går inte att ladda ned.";
        case 14:
            return "Kan ikke downloade.";
        case 15:
            return "Kan ikke laste ned.";
        case 16:
            return "Nie można pobrać zawartości.";
        case 17:
            return "Não é possível baixar.";
        case 19:
            return "İndirilemiyor.";
        case 20:
            return "No se ha podido descargar.";
        case 21:
            return "لا يمكن التنزيل.";
        case 22:
            return "Téléchargement impossible.";
        case 23:
            return "Stažení se nezdařilo.";
        case 24:
            return "Nem lehet letölteni.";
        case 25:
            return "Δεν είναι δυνατή η λήψη.";
        case 26:
            return "Nu se poate descărca.";
        case 27:
            return "ไม่สามารถดาวน์โหลดได้";
        case 28:
            return "Không thể tải về.";
        case 29:
            return "Tidak dapat mengunduh.";
        case 30:
            return "Не вдалося завантажити.";
        default:
            return "Cannot download.";
        }
    case 0x126:
        switch (lang) {
        case 0:
            return "インストールできませんでした。";
        case 2:
            return "Installation impossible.";
        case 3:
            return "No se ha podido instalar.";
        case 4:
            return "Installieren nicht möglich.";
        case 5:
            return "Impossibile installare.";
        case 6:
            return "Kan niet installeren.";
        case 7:
            return "Não é possível efetuar instalação.";
        case 8:
            return "Нельзя выполнить установку.";
        case 9:
            return "설치하지 못했습니다.";
        case 10:
            return "無法安裝。";
        case 11:
            return "无法安装。";
        case 12:
            return "Ei voida asentaa.";
        case 13:
            return "Det går inte att installera.";
        case 14:
            return "Kan ikke installere.";
        case 15:
            return "Kan ikke installere.";
        case 16:
            return "Nie można zainstalować.";
        case 17:
            return "Não é possível instalar.";
        case 19:
            return "Kurulamıyor.";
        case 20:
            return "No se ha podido instalar.";
        case 21:
            return "لا يمكن التثبيت.";
        case 22:
            return "Installation impossible.";
        case 23:
            return "Instalace se nezdařila.";
        case 24:
            return "Nem lehet telepíteni.";
        case 25:
            return "Δεν είναι δυνατή η εγκατάσταση.";
        case 26:
            return "Nu se poate instala.";
        case 27:
            return "ไม่สามารถติดตั้งได้";
        case 28:
            return "Không thể cài đặt.";
        case 29:
            return "Tidak dapat menginstal.";
        case 30:
            return "Не вдалося інсталювати.";
        default:
            return "Cannot install.";
        }
    case 0x12d:
        switch (lang) {
        case 0:
            return "インストールの準備ができました。";
        case 2:
            return "Prêt à installer.";
        case 3:
            return "Preparado para instalar.";
        case 4:
            return "Bereit zum Installieren.";
        case 5:
            return "Pronto per l'installazione.";
        case 6:
            return "Gereed om te installeren.";
        case 7:
            return "Pronto para efetuar a instalação.";
        case 8:
            return "Готово к установке.";
        case 9:
            return "설치할 준비가 되었습니다.";
        case 10:
            return "已準備好安裝。";
        case 11:
            return "已做好安装准备。";
        case 12:
            return "Valmis asennettavaksi.";
        case 13:
            return "Redo att installera.";
        case 14:
            return "Klar til at installere.";
        case 15:
            return "Klar til å installere.";
        case 16:
            return "Gotowe do zainstalowania.";
        case 17:
            return "Pronto para instalar.";
        case 19:
            return "Kuruluma hazır.";
        case 20:
            return "Preparado para instalar.";
        case 21:
            return "جاهز للتثبيت.";
        case 22:
            return "Prêt à installer.";
        case 23:
            return "Připraveno k instalaci.";
        case 24:
            return "Telepítésre kész.";
        case 25:
            return "Έτοιμο προς εγκατάσταση.";
        case 26:
            return "Instalarea este pregătită.";
        case 27:
            return "พร้อมติดตั้ง";
        case 28:
            return "Đã sẵn sàng để cài đặt.";
        case 29:
            return "Siap menginstal.";
        case 30:
            return "Готово до інсталяції.";
        default:
            return "Ready to install.";
        }
    case 0x127:
        switch (lang) {
        case 0:
            return "[アップロードリスト]に追加されました。";
        case 2:
            return "Ajouté à [Téléchargements en amont].";
        case 3:
            return "Añadido a [Cargas].";
        case 4:
            return "Zu [Uploads] hinzugefügt.";
        case 5:
            return "Aggiunto a [Caricamenti].";
        case 6:
            return "Toegevoegd aan [Uploads].";
        case 7:
            return "Adição efetuada a [Carregamentos].";
        case 8:
            return "Добавлено в [Загрузки в сеть].";
        case 9:
            return "[업로드 리스트]에 추가되었습니다.";
        case 10:
            return "已追加至[上傳內容]。";
        case 11:
            return "已添加至[上传]。";
        case 12:
            return "Lisätty kohteeseen [Lähetykset].";
        case 13:
            return "Tillagt i [Uppladdningar].";
        case 14:
            return "Tilføjet til [Uploads].";
        case 15:
            return "Lagt til [Opplastinger].";
        case 16:
            return "Dodano do [Przesłane].";
        case 17:
            return "Adicionado a [Uploads].";
        case 19:
            return "[Yüklenenler] seçeneğine eklendi.";
        case 20:
            return "Agregado a [Cargas].";
        case 21:
            return "تمت الإضافة إلى [التحميلات]‏.";
        case 22:
            return "Ajouté à [Téléchargements en amont].";
        case 23:
            return "Přidáno do [Nahrané soubory].";
        case 24:
            return "Hozzáadva [Feltöltések] tételhez.";
        case 25:
            return "Προστέθηκε στις [Αποστολές].";
        case 26:
            return "Adăugat la [Încărcări].";
        case 27:
            return "เพิ่มไปยัง [การอัพโหลด]";
        case 28:
            return "Đã thêm vào [Phần tải lên].";
        case 29:
            return "Ditambahkan ke [Unggahan].";
        case 30:
            return "Додано до [Передачі].";
        default:
            return "Added to [Uploads].";
        }
    case 0x128:
        switch (lang) {
        case 0:
            return "アップロードできませんでした。";
        case 2:
            return "Téléchargement en amont impossible.";
        case 3:
            return "No se ha podido subir.";
        case 4:
            return "Hochladen nicht möglich.";
        case 5:
            return "Impossibile caricare.";
        case 6:
            return "Kan niet uploaden.";
        case 7:
            return "Não é possível efetuar carregamento.";
        case 8:
            return "Нельзя выполнить загрузку в сеть.";
        case 9:
            return "업로드하지 못했습니다.";
        case 10:
            return "無法上傳。";
        case 11:
            return "无法上传。";
        case 12:
            return "Ei voida lähettää.";
        case 13:
            return "Det går inte att ladda upp.";
        case 14:
            return "Kan ikke uploade.";
        case 15:
            return "Kan ikke laste opp.";
        case 16:
            return "Nie można przesłać zawartości.";
        case 17:
            return "Não é possível fazer o upload.";
        case 19:
            return "Yüklenemiyor.";
        case 20:
            return "No se ha podido cargar.";
        case 21:
            return "لا يمكن التحميل.";
        case 22:
            return "Téléchargement en amont impossible.";
        case 23:
            return "Nahrání se nezdařilo.";
        case 24:
            return "Nem lehet feltölteni.";
        case 25:
            return "Δεν είναι δυνατή η αποστολή.";
        case 26:
            return "Nu se poate încărca.";
        case 27:
            return "ไม่สามารถอัพโหลด";
        case 28:
            return "Không thể tải lên.";
        case 29:
            return "Tidak dapat mengunggah.";
        case 30:
            return "Не вдалося передати.";
        default:
            return "Cannot upload.";
        }
    case 0x01a:
        switch (lang) {
        case 0:
            return "%1さんがPS4にログインしました。";
        case 2:
            return "%1 s'est connecté à la PS4.";
        case 3:
            return "%1 ha entrado en PS4.";
        case 4:
            return "%1 hat sich bei der PS4 eingeloggt.";
        case 5:
            return "%1 ha effettuato il login alla PS4.";
        case 6:
            return "%1 is ingelogd bij de PS4.";
        case 7:
            return "%1 iniciou uma sessão na PS4.";
        case 8:
            return "Пользователь %1 вошел в систему PS4.";
        case 9:
            return "%1님이 PS4에 로그인했습니다.";
        case 10:
            return "%1已登入PS4。";
        case 11:
            return "%1已登录PS4。";
        case 12:
            return "%1 on kirjautunut sisään PS4-järjestelmään.";
        case 13:
            return "%1 har loggat in på PS4-systemet.";
        case 14:
            return "%1 loggede ind på PS4.";
        case 15:
            return "%1 logget på PS4.";
        case 16:
            return "Użytkownik %1 zalogował się na PS4.";
        case 17:
            return "%1 fez login no PS4.";
        case 19:
            return "%1, PS4 sisteminde oturum açtı.";
        case 20:
            return "%1 entró en PS4.";
        case 21:
            return "قام ‎%1 بالدخول إلى جهاز PS4‎‏.";
        case 22:
            return "%1 s'est connecté à la PS4.";
        case 23:
            return "Uživatel %1 se přihlásil do systému PS4.";
        case 24:
            return "%1 bejelentkezett a PS4-be.";
        case 25:
            return "Ο/Η %1 συνδέθηκε στο PS4.";
        case 26:
            return "%1 conectat la PS4.";
        case 27:
            return "%1 เข้าสู่ระบบ PS4 แล้ว";
        case 28:
            return "%1 đã đăng nhập vào PS4.";
        case 29:
            return "%1 telah masuk ke PS4.";
        case 30:
            return "Користувач %1 увійшов до PS4.";
        default:
            return "%1 logged in to the PS4.";
        }
    case 0x133:
        switch (lang) {
        case 0:
            return "%1さんがオンラインになりました。";
        case 2:
            return "%1 est actuellement en ligne.";
        case 3:
            return "%1 está ahora online.";
        case 4:
            return "%1 ist jetzt online.";
        case 5:
            return "%1 è ora online.";
        case 6:
            return "%1 is nu online.";
        case 7:
            return "%1 está agora online.";
        case 8:
            return "%1 присутствует в сети.";
        case 9:
            return "%1님이 온라인했습니다.";
        case 10:
            return "%1已在線上。";
        case 11:
            return "%1上线了。";
        case 12:
            return "%1 on nyt online-tilassa.";
        case 13:
            return "%1 är nu online.";
        case 14:
            return "%1 er nu online.";
        case 15:
            return "%1 er nå online.";
        case 16:
            return "Użytkownik %1 jest teraz online.";
        case 17:
            return "%1 agora está online.";
        case 19:
            return "%1 şimdi çevrimiçi.";
        case 20:
            return "%1 está ahora online.";
        case 21:
            return "‎%1 متصل الآن.";
        case 22:
            return "%1 est actuellement en ligne.";
        case 23:
            return "Uživatel %1 je nyní online.";
        case 24:
            return "%1 online.";
        case 25:
            return "Ο/Η %1 είναι τώρα σε σύνδεση.";
        case 26:
            return "%1 este acum online.";
        case 27:
            return "ขณะนี้ %1 ออนไลน์อยู่";
        case 28:
            return "%1 hiện đang trực tuyến.";
        case 29:
            return "%1 sekarang online.";
        case 30:
            return "%1 зараз у мережі.";
        default:
            return "%1 is now online.";
        }
    case 0x04f:
        switch (lang) {
        case 0:
            return "%1さんがパーティーに参加しました。 (プレーヤー数: %2)";
        case 2:
            return "%1 a rejoint la Party. (Joueurs : %2)";
        case 3:
            return "%1 se ha unido al grupo. (Jugadores: %2)";
        case 4:
            return "%1 ist der Party beigetreten. (Spieler: %2)";
        case 5:
            return "%1 si è unito al party. (Giocatori: %2)";
        case 6:
            return "%1 is toegetreden tot de party. (spelers: %2)";
        case 7:
            return "%1 entrou no grupo. (Jogadores: %2)";
        case 8:
            return "Игрок %1 присоединился к тусовке. (Игроков: %2)";
        case 9:
            return "%1님이 파티에 참가했습니다. (플레이어 수: %2)";
        case 10:
            return "%1已參加派對。 （玩者人數：%2）";
        case 11:
            return "%1已参加派对。（玩家人数：%2）";
        case 12:
            return "%1 on liittynyt ryhmään. (Pelaajia: %2)";
        case 13:
            return "%1 har anslutit till partyt. (Spelare: %2)";
        case 14:
            return "%1 deltager i partyet. (Spillere: %2)";
        case 15:
            return "%1 er blitt med i gruppen. (Spillere: %2)";
        case 16:
            return "Użytkownik %1 dołączył do imprezy. (Gracze: %2)";
        case 17:
            return "%1 entrou na party. (Jogadores: %2)";
        case 19:
            return "%1 gruba katıldı. (Oyuncular: %2)";
        case 20:
            return "%1 se unió al grupo. (Jugadores: %2)";
        case 21:
            return "انضم ‎%1 إلى المجموعة. ‎(اللاعبون: ‎%2‎‏)";
        case 22:
            return "%1 a rejoint le groupe. (Joueurs : %2)";
        case 23:
            return "Uživatel %1 vstoupil do party. (Hráči: %2)";
        case 24:
            return "%1 csatlakozott a partihoz. (Játékosok: %2)";
        case 25:
            return "Ο/Η %1 συμμετέχει στο γκρουπ. (Παίκτες: %2)";
        case 26:
            return "%1 a intrat în gașcă. (Jucători: %2)";
        case 27:
            return "%1 เข้าร่วมปาร์ตี้แล้ว (ผู้เล่น: %2)";
        case 28:
            return "%1 đã tham gia đội chơi. (Người chơi: %2)";
        case 29:
            return "%1 bergabung dengan party. (Pemain: %2)";
        case 30:
            return "Гравець %1 приєднався до вечірки. (Гравці: %2)";
        default:
            return "%1 joined the party. (Players: %2)";
        }
    case 0x050:
        switch (lang) {
        case 0:
            return "%1さんがパーティーを退出しました。 (プレーヤー数: %2)";
        case 2:
            return "%1 a quitté la Party. (Joueurs : %2)";
        case 3:
            return "%1 ha abandonado el grupo. (Jugadores: %2)";
        case 4:
            return "%1 hat die Party verlassen. (Spieler: %2)";
        case 5:
            return "%1 ha abbandonato il party. (Giocatori: %2)";
        case 6:
            return "%1 heeft de party verlaten. (spelers: %2)";
        case 7:
            return "%1 saiu do grupo. (Jogadores: %2)";
        case 8:
            return "Игрок %1 покинул тусовку. (Игроков: %2)";
        case 9:
            return "%1님이 파티를 나갔습니다. (플레이어 수: %2)";
        case 10:
            return "%1已離開派對。 （玩者人數：%2）";
        case 11:
            return "%1已离开派对。（玩家人数：%2）";
        case 12:
            return "%1 on poistunut ryhmästä. (Pelaajia: %2)";
        case 13:
            return "%1 har lämnat partyt. (Spelare: %2)";
        case 14:
            return "%1 har forladt partyet. (Spillere: %2)";
        case 15:
            return "%1 forlot gruppen. (Spillere: %2)";
        case 16:
            return "Użytkownik %1 opuścił imprezę. (Gracze: %2)";
        case 17:
            return "%1 saiu da party. (Jogadores: %2)";
        case 19:
            return "%1 gruptan ayrıldı. (Oyuncular: %2)";
        case 20:
            return "%1 abandonó el grupo. (Jugadores: %2)";
        case 21:
            return "غادر ‎%1 المجموعة. ‎(اللاعبون: ‎%2‎‏)";
        case 22:
            return "%1 a quitté le groupe. (Joueurs : %2)";
        case 23:
            return "Uživatel %1 opustil partu. (Hráči: %2)";
        case 24:
            return "%1 kilépett a partiból. (Játékosok: %2)";
        case 25:
            return "Ο/Η %1 αποχώρησε από το γκρουπ. (Παίκτες: %2)";
        case 26:
            return "%1 a ieșit din gașcă. (Jucători: %2)";
        case 27:
            return "%1 ออกจากปาร์ตี้แล้ว (ผู้เล่น: %2)";
        case 28:
            return "%1 đã rời khỏi đội chơi. (Người chơi: %2)";
        case 29:
            return "%1 keluar dari party. (Pemain: %2)";
        case 30:
            return "Гравець %1 покинув вечірку. (Гравці: %2)";
        default:
            return "%1 left the party. (Players: %2)";
        }
    case 0x051:
        switch (lang) {
        case 0:
            return "パーティーのオーナーになりました。";
        case 2:
            return "Vous êtes désormais propriétaire d'une Party.";
        case 3:
            return "Ahora eres el propietario de un grupo.";
        case 4:
            return "Du bist jetzt Besitzer der Party.";
        case 5:
            return "Ora sei il proprietario di un party.";
        case 6:
            return "Je bent nu de eigenaar van een party.";
        case 7:
            return "É agora o proprietário de um grupo.";
        case 8:
            return "Теперь вы владелец тусовки.";
        case 9:
            return "이제 파티의 오너입니다.";
        case 10:
            return "您現在已成為派對的主辦人。";
        case 11:
            return "您现在已是派对的主人。";
        case 12:
            return "Olet nyt ryhmän omistaja.";
        case 13:
            return "Du är nu ägare till ett party.";
        case 14:
            return "Du er nu ejeren af et party.";
        case 15:
            return "Du er nå eieren av en gruppe.";
        case 16:
            return "Jesteś teraz właścicielem imprezy.";
        case 17:
            return "Agora você é o proprietário de uma party.";
        case 19:
            return "Artık grup sahibisiniz.";
        case 20:
            return "Ahora eres el propietario de un grupo.";
        case 21:
            return "أنت الآن مالك مجموعة.";
        case 22:
            return "Vous êtes désormais le propriétaire d’un groupe.";
        case 23:
            return "Nyní jste majitelem party.";
        case 24:
            return "Mostantól te vagy a parti tulajdonosa.";
        case 25:
            return "Είστε πλέον ιδιοκτήτης ενός γκρουπ.";
        case 26:
            return "Acum ești proprietarul găștii.";
        case 27:
            return "ตอนนี้คุณเป็นเจ้าของปาร์ตี้แล้ว";
        case 28:
            return "Bạn hiện là chủ sở hữu của một đội chơi.";
        case 29:
            return "Sekarang Anda adalah pemilik party.";
        case 30:
            return "Тепер ви власник вечірки.";
        default:
            return "You are now the owner of a party.";
        }
    case 0x0a8:
        switch (lang) {
        case 0:
            return "パーティーの招待を送信しました。";
        case 2:
            return "Invitation à rejoindre une Party envoyée.";
        case 3:
            return "Se ha enviado la invitación al grupo.";
        case 4:
            return "Partyeinladung gesendet.";
        case 5:
            return "Invito al party inviato.";
        case 6:
            return "Partyuitnodiging verzonden.";
        case 7:
            return "Convite para grupo enviado.";
        case 8:
            return "Приглашение на тусовку отправлено.";
        case 9:
            return "파티 초대를 보냈습니다.";
        case 10:
            return "已傳送派對邀請。";
        case 11:
            return "已发送派对邀请。";
        case 12:
            return "Kutsu ryhmään on lähetetty.";
        case 13:
            return "En partyinbjudan har skickats.";
        case 14:
            return "Partyinvitationen er sendt.";
        case 15:
            return "Gruppeinvitasjon er sendt.";
        case 16:
            return "Wysłano zaproszenie na imprezę.";
        case 17:
            return "Convite para a party enviado.";
        case 19:
            return "Grup daveti gönderildi.";
        case 20:
            return "Invitación al grupo enviada.";
        case 21:
            return "تم إرسال دعوة المجموعة.";
        case 22:
            return "Invitation à rejoindre un groupe envoyée.";
        case 23:
            return "Pozvánka do party byla odeslána.";
        case 24:
            return "Partimeghívás elküldve.";
        case 25:
            return "Η πρόσκληση σε γκρουπ στάλθηκε.";
        case 26:
            return "Invitația în gașcă a fost trimisă.";
        case 27:
            return "ส่งคำเชิญเข้าร่วมปาร์ตี้แล้ว";
        case 28:
            return "Đã gửi lời mời tham gia đội chơi.";
        case 29:
            return "Undangan party terkirim.";
        case 30:
            return "Запрошення на вечірку надіслано.";
        default:
            return "Party invitation sent.";
        }
    case 0x101:
        switch (lang) {
        case 0:
            return "パーティーに参加しました。";
        case 2:
            return "Party rejointe.";
        case 3:
            return "Se ha unido al grupo.";
        case 4:
            return "Beitritt zu Party erfolgt.";
        case 5:
            return "Partecipi al party.";
        case 6:
            return "Toegetreden tot party.";
        case 7:
            return "Entrou no grupo.";
        case 8:
            return "Присоединение к тусовке состоялось.";
        case 9:
            return "파티에 참가했습니다.";
        case 10:
            return "已參加派對。";
        case 11:
            return "已参加派对。";
        case 12:
            return "Liityit ryhmään.";
        case 13:
            return "Ansluten till partyt.";
        case 14:
            return "Deltager i party.";
        case 15:
            return "Med i gruppe.";
        case 16:
            return "Dołączono do imprezy.";
        case 17:
            return "Entrou na party.";
        case 19:
            return "Gruba katıldı.";
        case 20:
            return "Se ha unido al grupo.";
        case 21:
            return "تم الانضمام للمجموعة.";
        case 22:
            return "Groupe rejoint.";
        case 23:
            return "Připojeni do party.";
        case 24:
            return "Csatlakoztál a partihoz.";
        case 25:
            return "Συμμετέχετε στο γκρουπ.";
        case 26:
            return "Ai intrat în gașcă.";
        case 27:
            return "เข้าร่วมปาร์ตี้แล้ว";
        case 28:
            return "Đã tham gia đội chơi.";
        case 29:
            return "Party yang diikuti.";
        case 30:
            return "Приєдналися до вечірки.";
        default:
            return "Party joined.";
        }
    case 0x0f0:
        switch (lang) {
        case 0:
            return "パーティーを退出しました。";
        case 2:
            return "Vous avez quitté la Party.";
        case 3:
            return "Has abandonado el grupo.";
        case 4:
            return "Du hast die Party verlassen.";
        case 5:
            return "Hai abbandonato il party.";
        case 6:
            return "Je hebt de party verlaten.";
        case 7:
            return "Saiu do grupo.";
        case 8:
            return "Вы покинули тусовку.";
        case 9:
            return "파티를 나갔습니다.";
        case 10:
            return "您已離開派對。";
        case 11:
            return "您已离开派对。";
        case 12:
            return "Olet poistunut ryhmästä.";
        case 13:
            return "Du har lämnat partyt.";
        case 14:
            return "Du har forladt partyet.";
        case 15:
            return "Du har forlatt gruppen.";
        case 16:
            return "Opuszczasz imprezę.";
        case 17:
            return "Você saiu da party.";
        case 19:
            return "Gruptan ayrıldınız.";
        case 20:
            return "Abandonaste el grupo.";
        case 21:
            return "لقد غادرت المجموعة.";
        case 22:
            return "Vous avez quitté le groupe.";
        case 23:
            return "Opustili jste partu.";
        case 24:
            return "Kiléptél a partiból.";
        case 25:
            return "Έχετε αποχωρήσει από το γκρουπ.";
        case 26:
            return "Ai ieșit din gașcă.";
        case 27:
            return "คุณออกจากปาร์ตี้แล้ว";
        case 28:
            return "Bạn đã rời khỏi đội chơi.";
        case 29:
            return "Anda telah keluar dari party ini.";
        case 30:
            return "Ви покинули вечірку.";
        default:
            return "You have left the party.";
        }
    case 0x019:
        switch (lang) {
        case 0:
            return "ネットワークに接続できませんでした。";
        case 2:
            return "Connexion au réseau impossible.";
        case 3:
            return "No se puede conectar a la red.";
        case 4:
            return "Netzwerkverbindung nicht möglich.";
        case 5:
            return "Impossibile connettersi alla rete.";
        case 6:
            return "Kan geen verbinding maken met het netwerk.";
        case 7:
            return "Não é possível estabelecer uma ligação à rede.";
        case 8:
            return "Не удалось подключиться к сети.";
        case 9:
            return "네트워크에 접속하지 못했습니다.";
        case 10:
            return "無法與網路連線。";
        case 11:
            return "无法与网络连接。";
        case 12:
            return "Verkkoon ei saatu yhteyttä.";
        case 13:
            return "Det går inte att ansluta till nätverket.";
        case 14:
            return "Kan ikke oprette forbindelse til netværket.";
        case 15:
            return "Kan ikke koble til nettverket.";
        case 16:
            return "Nie można nawiązać połączenia z siecią.";
        case 17:
            return "Não é possível conectar-se à rede.";
        case 19:
            return "Ağa bağlanılamıyor.";
        case 20:
            return "No se puede conectar a la red.";
        case 21:
            return "تعذر الاتصال بالشبكة.";
        case 22:
            return "Connexion au réseau impossible.";
        case 23:
            return "Nelze se připojit k síti.";
        case 24:
            return "Nem lehet kapcsolódni hálózathoz.";
        case 25:
            return "Δεν είναι δυνατή η σύνδεση στο δίκτυο.";
        case 26:
            return "Nu se poate conecta la rețea.";
        case 27:
            return "ไม่สามารถเชื่อมต่อกับเครือข่าย";
        case 28:
            return "Không thể kết nối mạng.";
        case 29:
            return "Tidak dapat tersambung ke jaringan.";
        case 30:
            return "Не вдалося підключитися до мережі.";
        default:
            return "Cannot connect to network.";
        }
    case 0x01f:
        switch (lang) {
        case 0:
            return "現在、PlayStation Networkは混雑しています。";
        case 2:
            return "PlayStation Network est occupé.";
        case 3:
            return "PlayStation Network está ocupado en estos momentos.";
        case 4:
            return "Das PlayStation Network ist zurzeit ausgelastet.";
        case 5:
            return "PlayStation Network è attualmente occupato.";
        case 6:
            return "PlayStation Network is momenteel bezig.";
        case 7:
            return "De momento, a PlayStation Network está ocupada.";
        case 8:
            return "Сеть PlayStation Network сейчас занята.";
        case 9:
            return "현재, PlayStation Network로의 접속이 지연되고 있습니다.";
        case 10:
            return "PlayStation Network目前忙線中。";
        case 11:
            return "PlayStation Network正处繁忙。";
        case 12:
            return "PlayStation Network on parhaillaan varattu.";
        case 13:
            return "PlayStation Network är upptaget.";
        case 14:
            return "PlayStation Network er i øjeblikket optaget.";
        case 15:
            return "PlayStation Network er i øyeblikket opptatt.";
        case 16:
            return "Sieć PlayStation Network jest zajęta.";
        case 17:
            return "A PlayStation Network está ocupada no momento.";
        case 19:
            return "PlayStation Network meşgul.";
        case 20:
            return "PlayStation Network está ocupado.";
        case 21:
            return "PlayStation Network مشغولة حاليًا.";
        case 22:
            return "PlayStation Network est occupé.";
        case 23:
            return "Služba PlayStation Network je v současné době zaneprázdněná.";
        case 24:
            return "APlayStation Network jelenleg foglalt.";
        case 25:
            return "Το PlayStation Network είναι επί του παρόντος απασχολημένο.";
        case 26:
            return "PlayStation Network este ocupat temporar.";
        case 27:
            return "ขณะนี้ PlayStation Network ไม่ว่าง";
        case 28:
            return "PlayStation Network hiện đang bận.";
        case 29:
            return "PlayStation Network sekarang sedang sibuk.";
        case 30:
            return "Зараз служба PlayStation Network зайнята.";
        default:
            return "PlayStation Network is currently busy.";
        }
    case 0x020:
        switch (lang) {
        case 0:
            return "現在、PlayStation Networkはメンテナンス中です。";
        case 2:
            return "PlayStation Network est en cours de maintenance.";
        case 3:
            return "PlayStation Network está llevando a cabo labores de mantenimiento en estos "
                   "momentos.";
        case 4:
            return "Das PlayStation Network wird zurzeit gewartet.";
        case 5:
            return "PlayStation Network è attualmente in manutenzione.";
        case 6:
            return "Momenteel vindt onderhoud plaats op het PlayStation Network.";
        case 7:
            return "De momento, a PlayStation Network está em manutenção.";
        case 8:
            return "В настоящий момент проводится техническое обслуживание PlayStation Network.";
        case 9:
            return "현재 PlayStation Network는 점검중입니다.";
        case 10:
            return "PlayStation Network目前維修中。";
        case 11:
            return "PlayStation Network目前正在维护。";
        case 12:
            return "PlayStation Network -verkkoa huolletaan parhaillaan.";
        case 13:
            return "PlayStation Network underhålls för närvarande.";
        case 14:
            return "PlayStation Network er i øjeblikket under vedligeholdelse.";
        case 15:
            return "Vedlikehold pågår på PlayStation Network.";
        case 16:
            return "Aktualnie trwa konserwacja sieci PlayStation Network.";
        case 17:
            return "A PlayStation Network está em manutenção no momento.";
        case 19:
            return "PlayStation Network bakım aşamasında.";
        case 20:
            return "PlayStation Network se encuentra en mantenimiento actualmente.";
        case 21:
            return "تجري حاليًا صيانة PlayStation Network‏.";
        case 22:
            return "PlayStation Network est en cours de maintenance.";
        case 23:
            return "Služba PlayStation Network v současné době prochází údržbou.";
        case 24:
            return "APlayStation Network jelenleg karbantartás alatt van.";
        case 25:
            return "Το PlayStation Network επί του παρόντος υποβάλλεται σε συντήρηση.";
        case 26:
            return "PlayStation Network este în prezent în curs de întreținere.";
        case 27:
            return "ขณะนี้ PlayStation Network กำลังอยู่ระหว่างการปรับปรุงระบบ";
        case 28:
            return "PlayStation Network hiện đang được bảo trì.";
        case 29:
            return "PlayStation Network sekarang sedang dalam pemeliharaan.";
        case 30:
            return "Зараз відбувається технічне обслуговування PlayStation Network.";
        default:
            return "PlayStation Network is currently undergoing maintenance.";
        }
    case 0x023:
        switch (lang) {
        case 0:
            return "ネットワーク機能を使うには、PSNにサインインしてください。";
        case 2:
            return "Connectez-vous à PSN pour utiliser les fonctionnalités de réseau.";
        case 3:
            return "Inicia sesión en PSN para usar las funciones de red.";
        case 4:
            return "Melde dich beim PSN an, um Netzwerkfunktionen zu verwenden.";
        case 5:
            return "Per usare le funzioni di rete, accedi a PSN.";
        case 6:
            return "Meld je aan bij PSN om netwerkfuncties te gebruiken.";
        case 7:
            return "Inicie uma sessão na PSN para utilizar as funcionalidades de rede.";
        case 8:
            return "Войдите в сеть PSN для использования сетевых функций.";
        case 9:
            return "네트워크 기능을 이용하려면 PSN에 로그인해야 합니다.";
        case 10:
            return "若要使用網路機能，請先登入PSN。";
        case 11:
            return "若要使用网络功能，请先登陆PSN。";
        case 12:
            return "Kirjaudu sisään PSN-verkkoon, jotta voit käyttää verkko-ominaisuuksia.";
        case 13:
            return "Logga in på PSN om du vill använda nätverksfunktionerna.";
        case 14:
            return "Log ind på PSN for at anvende netværksfunktioner.";
        case 15:
            return "Logg på PSN for å bruke nettverksfunksjoner.";
        case 16:
            return "Aby korzystać z funkcji sieciowych, wpisz się do sieci PSN.";
        case 17:
            return "Inicie uma sessão na PSN para usar os recursos de rede.";
        case 19:
            return "Ağ özelliklerini kullanmak için PSN'e giriş yapın.";
        case 20:
            return "Inicia sesión en PSN para usar las funciones de red.";
        case 21:
            return "سجّل الدخول إلى PSN لاستخدام ميزات الشبكة.";
        case 22:
            return "Connectez-vous à PSN pour utiliser les fonctionnalités de réseau.";
        case 23:
            return "Chcete-li používat síťové funkce, přihlaste se do služby PSN.";
        case 24:
            return "Jelentkezz be a PSN-re a hálózati funkciók használatához.";
        case 25:
            return "Συνδεθείτε στο PSN για να χρησιμοποιήσετε τις δικτυακές δυνατότητες.";
        case 26:
            return "Conectează-te la PSN pentru a utiliza caracteristicile de rețea.";
        case 27:
            return "ลงชื่อเข้าใช้ PSN เพื่อใช้คุณสมบัติเครือข่าย";
        case 28:
            return "Đăng nhập PSN để sử dụng các tính năng mạng.";
        case 29:
            return "Masuk ke PSN untuk menggunakan fitur jaringan.";
        case 30:
            return "Увійдіть до PSN, щоб використовувати мережеві функції.";
        default:
            return "Sign in to PSN to use network features.";
        }
    case 0x024:
        switch (lang) {
        case 0:
            return "PSNからサインアウトしました。";
        case 2:
            return "Déconnecté de PSN.";
        case 3:
            return "Se ha cerrado la sesión de PSN.";
        case 4:
            return "Abgemeldet vom PSN.";
        case 5:
            return "Uscito da PSN.";
        case 6:
            return "Je bent afgemeld bij PSN.";
        case 7:
            return "Sessão na PSN finalizada.";
        case 8:
            return "Выход из сети PSN.";
        case 9:
            return "PSN에서 로그아웃했습니다.";
        case 10:
            return "已登出PSN。";
        case 11:
            return "已登出PSN。";
        case 12:
            return "Olet kirjautunut ulos PSN-verkosta.";
        case 13:
            return "Utloggad från PSN.";
        case 14:
            return "Loggede ud af PSN.";
        case 15:
            return "Logget av PSN.";
        case 16:
            return "Wypisano z sieci PSN.";
        case 17:
            return "Sessão na PSN finalizada.";
        case 19:
            return "PSN'den çıkış yapıldı.";
        case 20:
            return "Se cerró la sesión de PSN.";
        case 21:
            return "تم تسجيل الخروج من PSN‏.";
        case 22:
            return "Déconnecté de PSN.";
        case 23:
            return "Odhlášeno ze služby PSN.";
        case 24:
            return "Kijelentkezve PSN-ről.";
        case 25:
            return "Έξοδος από το PSN.";
        case 26:
            return "Deconectat de la PSN.";
        case 27:
            return "ออกจากระบบ PSN แล้ว";
        case 28:
            return "Đã đăng xuất khỏi PSN.";
        case 29:
            return "Sudah keluar dari PSN.";
        case 30:
            return "Виконано вихід із PSN.";
        default:
            return "Signed out of PSN.";
        }
    case 0x01c:
        switch (lang) {
        case 0:
            return "あなたのアカウントでは、PlayStation Networkへのアクセスが禁止されています。";
        case 2:
            return "Votre compte a été définitivement suspendu sur PlayStation Network.";
        case 3:
            return "El acceso de tu cuenta a PlayStation Network se ha suspendido permanentemente.";
        case 4:
            return "Der Zugriff auf PlayStation Network über dein Konto wurde dauerhaft "
                   "ausgesetzt.";
        case 5:
            return "Il tuo account è definitivamente sospeso da PlayStation Network.";
        case 6:
            return "Je account is permanent opgeschort op PlayStation Network.";
        case 7:
            return "A sua conta está suspensa permanentemente da PlayStation Network.";
        case 8:
            return "Доступ в сеть PlayStation Network из вашей учетной записи окончательно "
                   "заблокирован.";
        case 9:
            return "내 계정이 PlayStation Network에서 영구 정지되었습니다.";
        case 10:
            return "您的帳戶已被永久停權，無法存取PlayStation Network。";
        case 11:
            return "您的账号已被永久停用存取PlayStation Network。";
        case 12:
            return "Pääsy PlayStation Network -verkkoon tililtäsi on estetty pysyvästi.";
        case 13:
            return "Ditt konto har stängts av permanent från PlayStation Network.";
        case 14:
            return "Din konto er permanent suspenderet fra PlayStation Network.";
        case 15:
            return "Tilgang til PlayStation Network er permanent sperret for kontoen din.";
        case 16:
            return "Dostęp do PlayStation Network z Twojego konta został trwale zawieszony.";
        case 17:
            return "Sua conta foi suspensa permanentemente da PlayStation Network.";
        case 19:
            return "Hesabınızın PlayStation Network erişimi süresiz olarak askıya alındı.";
        case 20:
            return "Tu cuenta se ha suspendido permanentemente de PlayStation Network.";
        case 21:
            return "تم تعليق وصول حسابك إلى PlayStation Network بصفة دائمة.";
        case 22:
            return "Votre compte a été définitivement suspendu sur PlayStation Network.";
        case 23:
            return "Váš účet je ve službě PlayStation Network trvale zablokován.";
        case 24:
            return "A fiókod véglegesen fel lett függesztve a PlayStation Network hálózatban.";
        case 25:
            return "Ο λογαριασμός σας έχει ανασταλεί μόνιμα στο PlayStation Network.";
        case 26:
            return "Contul tău este suspendat permanent pe PlayStation Network.";
        case 27:
            return "บัญชีของคุณถูกระงับถาวรจาก PlayStation Network";
        case 28:
            return "Tài khoản của bạn vĩnh viễn bị đình chỉ trên PlayStation Network.";
        case 29:
            return "Akun Anda ditangguhkan selamanya dari PlayStation Network.";
        case 30:
            return "Вашому обліковому запису остаточно заблоковано доступ до PlayStation Network.";
        default:
            return "Your account is permanently suspended from PlayStation Network.";
        }
    case 0x0ef:
        switch (lang) {
        case 0:
            return "あなたのアカウントでは、PlayStation "
                   "Networkへのアクセスが一時的に停止されています。";
        case 2:
            return "L'accès à PlayStation Network a été suspendu pour votre compte.";
        case 3:
            return "Tu cuenta ha sido suspendida en PlayStation Network.";
        case 4:
            return "Der Zugriff deines Kontos auf PlayStation Network wurde vorübergehend "
                   "ausgesetzt.";
        case 5:
            return "Il tuo account è stato sospeso da PlayStation Network.";
        case 6:
            return "Je account is opgeschort bij PlayStation Network.";
        case 7:
            return "A sua conta foi suspensa da PlayStation Network.";
        case 8:
            return "Доступ к PlayStation Network из вашей учетной записи приостановлен.";
        case 9:
            return "고객님의 계정은 PlayStation Network 접속이 일시 정지되어 있습니다.";
        case 10:
            return "您的帳戶已被暫時停止存取PlayStation Network。";
        case 11:
            return "您的账号已被暂停存取PlayStation Network。";
        case 12:
            return "Pääsy PlayStation Network -verkkoon tililtäsi on väliaikaisesti estetty.";
        case 13:
            return "Ditt konto har stängts av från PlayStation Network.";
        case 14:
            return "Din konto er suspenderet fra PlayStation Network.";
        case 15:
            return "Kontoen din er sperret fra PlayStation Network.";
        case 16:
            return "Dostęp do sieci PlayStation Network z Twojego konta został zawieszony.";
        case 17:
            return "Sua conta foi suspensa da PlayStation Network.";
        case 19:
            return "Hesabınız PlayStation Network'ten askıya alındı.";
        case 20:
            return "Tu cuenta fue suspendida en PlayStation Network.";
        case 21:
            return "تم تعليق الوصول إلى حسابك من PlayStation Network‏.";
        case 22:
            return "L'accès à PlayStation Network a été suspendu pour votre compte.";
        case 23:
            return "Váš účet je ve službě PlayStation Network pozastaven.";
        case 24:
            return "A fiókod fel van függesztve a PlayStation Network hálózaton.";
        case 25:
            return "Ο λογαριασμός σας έχει ανασταλεί στο PlayStation Network.";
        case 26:
            return "Accesul contului tău la PlayStation Network este suspendat.";
        case 27:
            return "บัญชีของคุณถูกระงับชั่วคราวจาก PlayStation Network";
        case 28:
            return "Tài khoản của bạn bị treo không được vào PlayStation Network.";
        case 29:
            return "Akun Anda ditangguhkan dari PlayStation Network.";
        case 30:
            return "Доступ до PlayStation Network із вашого облікового запису призупинено.";
        default:
            return "Your account is suspended from PlayStation Network.";
        }
    default:
        return {};
    }
}

static std::string FormatSystemMessage(std::string_view tpl, std::string_view p1,
                                       std::string_view p2, std::string_view p3) {
    std::string out;
    out.reserve(tpl.size() + p1.size() + p2.size());
    for (size_t i = 0; i < tpl.size(); ++i) {
        if (tpl[i] == '%' && i + 1 < tpl.size()) {
            const char idx = tpl[i + 1];
            if (idx == '1' || idx == '2' || idx == '3') {
                const std::string_view rep = (idx == '1') ? p1 : (idx == '2') ? p2 : p3;
                out += rep;
                ++i;
                continue;
            }
        }
        out += tpl[i];
    }
    return out;
}

struct ExtractedNotification {
    std::string user_name;
    std::string target_user;
    std::string title;
    std::string message;
};

static ExtractedNotification ExtractNotificationText(const OrbisNotificationRequest& req) {
    auto slice = [&](size_t offset, size_t max_len) -> std::string {
        if (offset >= sizeof(req.message)) {
            return {};
        }
        const size_t avail = std::min(max_len, sizeof(req.message) - offset);
        const char* ptr = req.message + offset;
        return std::string(ptr, strnlen(ptr, avail));
    };

    const bool has_three_slots = (req.type == 9 || req.type == 11 || req.type == 13);
    const bool has_short_slot =
        (req.type == 5 || req.type == 6 || req.type == 10 || req.type == 12 || req.type == 16);
    const bool is_user_type =
        (req.type == 2 || req.type == 4 || req.type == 6 || req.type == 10 || req.type == 12 ||
         req.type == 13 || req.type == 16 || req.type == 17);
    const bool is_device_type = (req.type == 3 || req.type == 4 || req.type == 15);
    const bool is_user_panel_msg =
        is_user_type || req.msg_id == 0x01c || req.msg_id == 0x0ef || req.msg_id == 0x04f ||
        req.msg_id == 0x050 || req.msg_id == 0x051 || req.msg_id == 0x0a8 || req.msg_id == 0x0f0 ||
        req.msg_id == 0x101 || req.msg_id == 0x133;

    std::string s0;
    std::string s1;
    std::string s2;
    if (has_three_slots) {
        constexpr size_t kSlotSize = 0xb4;
        s0 = slice(0, kSlotSize);
        s1 = slice(kSlotSize, kSlotSize);
        s2 = (req.type == 11) ? slice(kSlotSize * 2, kSlotSize) : std::string{};
    } else if (req.type != 0) {
        s0 = slice(0, has_short_slot ? 0xb4 : sizeof(req.message));
        if ((req.type == 12 || req.type == 16) && sizeof(req.message) > 0xb4) {
            s1 = slice(0xb4, 0xb4);
        }
    } else {
        s0 = slice(0, sizeof(req.message));
    }

    int logged_in_count = 0;
    auto resolve_local_user_name = [&]() -> std::string {
        if (req.user_id > 0) {
            if (auto* user = UserManagement.GetUserByID(req.user_id)) {
                return user->user_name;
            }
        }
        for (auto* user : UserManagement.GetLoggedInUsers()) {
            if (user && !user->user_name.empty()) {
                return user->user_name;
            }
        }
        const User def_user = UserManagement.GetDefaultUser();
        if (!def_user.user_name.empty()) {
            return def_user.user_name;
        }
        return {};
    };
    for (auto* user : UserManagement.GetLoggedInUsers()) {
        if (user) {
            ++logged_in_count;
        }
    }

    const std::string local_user_name =
        is_user_panel_msg ? resolve_local_user_name() : std::string{};
    // PS4 only shows the top UserNamePanel on regular toasts in multi-user sessions (>1 signed-in
    // user).
    std::string user_name = (logged_in_count > 1) ? local_user_name : std::string{};
    std::string target_user;
    if (!s0.empty() && (req.type == 12 || req.type == 13 || req.type == 16 || req.msg_id == 0x133 ||
                        req.msg_id == 0x01a)) {
        target_user = s0;
    }

    if (req.type != 0) {
        const s32 lang = EmulatorSettings.GetConsoleLanguage();
        if (const std::string_view tpl = LookupSystemMessage(req.msg_id, lang); !tpl.empty()) {
            std::string p1 =
                (has_three_slots || has_short_slot || is_device_type) ? s0 : std::string{};
            std::string p2 = has_three_slots ? s1 : std::string{};
            std::string p3 = has_three_slots ? s2 : std::string{};
            if (p1.empty() && is_user_panel_msg) {
                p1 = !local_user_name.empty() ? local_user_name : std::string("Player");
            } else if (p1.empty() && is_device_type) {
                p1 = "DUALSHOCK 4";
            }
            if (p2.empty() && tpl.find("%2") != std::string_view::npos) {
                p2 = "1";
            }

            std::string body = FormatSystemMessage(tpl, p1, p2, p3);
            std::string title;
            if ((req.type == 9 || req.type == 13) && !s0.empty() &&
                tpl.find("%1") == std::string_view::npos && s0 != local_user_name) {
                title = std::move(s0);
            }
            if (req.error_num != 0) {
                body += fmt::format(" ({:08X})", static_cast<u32>(req.error_num));
            }
            return {std::move(user_name), std::move(target_user), std::move(title),
                    std::move(body)};
        }
    }

    if (has_three_slots) {
        std::string body = std::move(s1);
        if (!s2.empty()) {
            if (!body.empty()) {
                body += ' ';
            }
            body += s2;
        }
        if (body.empty()) {
            return {std::move(user_name), std::move(target_user), {}, std::move(s0)};
        }
        if (!user_name.empty() && s0 == user_name) {
            return {std::move(user_name), std::move(target_user), {}, std::move(body)};
        }
        return {std::move(user_name), std::move(target_user), std::move(s0), std::move(body)};
    }

    if ((req.type == 12 || req.type == 16) && !s0.empty() && !s1.empty()) {
        return {std::move(user_name), std::move(target_user), {}, std::move(s1)};
    }

    if (s0.empty() && req.type == 7 && req.error_num != 0) {
        s0 = fmt::format("Error {:#010x}", static_cast<u32>(req.error_num));
    } else if (s0.empty() && req.type == 8 && req.app_id != 0) {
        s0 = fmt::format("Application {:#010x}", static_cast<u32>(req.app_id));
    } else if (s0.empty()) {
        return {};
    }
    return {std::move(user_name), std::move(target_user), {}, std::move(s0)};
}

static std::pair<Libraries::SystemNotification::NotificationIcon,
                 Libraries::SystemNotification::NotificationSound>
ResolveNotificationStyle(const OrbisNotificationRequest& req) {
    using Icon = Libraries::SystemNotification::NotificationIcon;
    using Sound = Libraries::SystemNotification::NotificationSound;

    const s32 id = req.msg_id;
    Sound sound = Sound::System;
    if (id == 0x062) {
        sound = Sound::Capture;
    } else if (id == 0x04f) {
        sound = Sound::PartyEnter;
    } else if (id == 0x050 || id == 0x0f0) {
        sound = Sound::PartyQuit;
    } else if (req.type == 3 || req.type == 4 || req.type == 15 || id == 0x004 || id == 0x005 ||
               id == 0x04b || id == 0x04d || id == 0x0d2 || id == 0x166) {
        sound = Sound::PsButton;
    } else if (req.type == 0) {
        sound = Sound::Notification;
    }

    Icon icon = Icon::System;
    if (id == 0x062 || id == 0x030) {
        icon = Icon::Capture;
    } else if (id == 0x122 || (id >= 0x124 && id <= 0x126) || id == 0x12d) {
        icon = Icon::Download;
    } else if (id == 0x127 || id == 0x128) {
        icon = Icon::Upload;
    } else if (id == 0x01c || id == 0x0ef || id == 0x019 || id == 0x01f || id == 0x020 ||
               id == 0x023 || id == 0x024) {
        icon = Icon::Error;
    } else if (id == 0x004 || id == 0x005 || id == 0x04b || id == 0x04d || id == 0x0d2 ||
               id == 0x166) {
        icon = Icon::System;
    } else if (id == 0x04f || id == 0x050 || id == 0x051 || id == 0x0a8 || id == 0x0f0 ||
               id == 0x101) {
        icon = Icon::Party;
    } else if (id == 0x01a || id == 0x133) {
        icon = Icon::User;
    } else {
        switch (req.type) {
        case 2:
        case 6:
        case 10:
        case 12:
        case 13:
        case 16:
        case 17:
            icon = Icon::User;
            break;
        case 3:
        case 4:
        case 15:
            icon = Icon::System;
            break;
        case 7:
            icon = Icon::Error;
            break;
        case 8:
        case 9:
            icon = Icon::App;
            break;
        default:
            break;
        }
    }
    return {icon, sound};
}

static std::vector<u8> LoadNotificationIcon(const OrbisNotificationRequest& req) {
    if (req.use_icon_image_uri == 0 || req.icon_uri[0] == '\0') {
        return {};
    }
    std::string_view uri(req.icon_uri, strnlen(req.icon_uri, sizeof(req.icon_uri)));
    if (uri.starts_with("file://")) {
        uri.remove_prefix(7);
    }
    if (!uri.starts_with('/')) {
        return {};
    }
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    auto file = mnt->Open(uri, Common::FS::FileAccessMode::Read);
    if (!file || !file->IsOpen()) {
        return {};
    }
    const u64 file_size = file->Size();
    if (file_size == 0 || file_size > 16 * 1024 * 1024) {
        return {};
    }
    std::vector<u8> data(file_size);
    const s64 bytes_read = file->Read(data.data(), file_size);
    if (bytes_read <= 0) {
        return {};
    }
    data.resize(static_cast<size_t>(bytes_read));
    return data;
}

s32 PS4_SYSV_ABI sceKernelSendNotificationRequest(s32 device, const OrbisNotificationRequest* req,
                                                  size_t size, s32 blocking) {
    if (device < 0 || device > 1) {
        LOG_ERROR(Lib_Kernel, "sceKernelSendNotificationRequest: invalid device={}", device);
        return ORBIS_KERNEL_ERROR_EBADF;
    }
    if (!req || size < sizeof(OrbisNotificationRequest)) {
        LOG_ERROR(Lib_Kernel,
                  "sceKernelSendNotificationRequest: invalid argument req={}, size={:#x}",
                  fmt::ptr(req), size);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto ext = ExtractNotificationText(*req);
    LOG_INFO(Lib_Kernel,
             "sceKernelSendNotificationRequest: device={}, type={}, msg_id={}, user={}, "
             "title='{}', msg='{}', blocking={}",
             device, req->type, req->msg_id, ext.user_name, ext.title, ext.message, blocking);

    auto [icon_type, sound_type] = ResolveNotificationStyle(*req);
    if (req->msg_id == 0x062) {
        ext.title.clear();
        ext.message.clear();
    }
    if (!ext.title.empty() || !ext.message.empty() ||
        icon_type == Libraries::SystemNotification::NotificationIcon::Capture) {
        auto icon_data = LoadNotificationIcon(*req);
        Libraries::SystemNotification::AddNotificationToQueue(
            ext.message, ext.title, {}, std::move(icon_data), icon_type, sound_type, req->user_id,
            ext.user_name, Libraries::SystemNotification::BatteryStatus::None, ext.target_user);
    }
    return ORBIS_OK;
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
    LIB_FUNCTION("zl7hupSO0C0", "libkernel", 1, "libkernel", sceKernelSendNotificationRequest);
}

} // namespace Libraries::Kernel
