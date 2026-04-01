// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/linker.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

void ErrSceToPosix(s32 result);
s32 ErrnoToSceKernelError(s32 e);
void SetPosixErrno(s32 e);
s32* PS4_SYSV_ABI __Error();
const char* PS4_SYSV_ABI sceKernelGetFsSandboxRandomWord();

extern Core::EntryParams entry_params;

template <class F, F f>
struct OrbisWrapperImpl;

template <class R, class... Args, PS4_SYSV_ABI R (*f)(Args...)>
struct OrbisWrapperImpl<PS4_SYSV_ABI R (*)(Args...), f> {
    static R PS4_SYSV_ABI wrap(Args... args) {
        u32 ret = f(args...);
        if (ret != 0) {
            ret += ORBIS_KERNEL_ERROR_UNKNOWN;
        }
        return ret;
    }
};

#define ORBIS(func) (Libraries::Kernel::OrbisWrapperImpl<decltype(&(func)), func>::wrap)

#define CURRENT_FIRMWARE_VERSION 0x13500011

s32* PS4_SYSV_ABI __Error();

struct SwVersionStruct {
    u64 struct_size;
    char text_representation[0x1c];
    u32 hex_representation;
};

struct AuthInfoData {
    u64 paid;
    u64 caps[4];
    u64 attrs[4];
    u64 ucred[8];
};

struct OrbisKernelTitleWorkaround {
    s32 version;
    s32 align;
    u64 ids[2];
};

struct OrbisKernelAppInfo {
    s32 app_id;
    s32 mmap_flags;
    s32 attribute_exe;
    s32 attribute2;
    char cusa_name[10];
    u8 debug_level;
    u8 slv_flags;
    u8 mini_app_dmem_flags;
    u8 render_mode;
    u8 mdbg_out;
    u8 required_hdcp_type;
    u64 preload_prx_flags;
    s32 attribute1;
    s32 has_param_sfo;
    OrbisKernelTitleWorkaround title_workaround;
};

void RegisterLib(Core::Loader::SymbolsResolver* sym);

constexpr u32 POSIX_SC_ARG_MAX = 1;
constexpr u32 POSIX_SC_CHILD_MAX = 2;
constexpr u32 POSIX_SC_CLK_TCK = 3;
constexpr u32 POSIX_SC_NGROUPS_MAX = 4;
constexpr u32 POSIX_SC_OPEN_MAX = 5;
constexpr u32 POSIX_SC_JOB_CONTROL = 6;
constexpr u32 POSIX_SC_SAVED_IDS = 7;
constexpr u32 POSIX_SC_VERSION = 8;
constexpr u32 POSIX_SC_BC_BASE_MAX = 9;
constexpr u32 POSIX_SC_BC_DIM_MAX = 10;
constexpr u32 POSIX_SC_BC_SCALE_MAX = 11;
constexpr u32 POSIX_SC_BC_STRING_MAX = 12;
constexpr u32 POSIX_SC_COLL_WEIGHTS_MAX = 13;
constexpr u32 POSIX_SC_EXPR_NEST_MAX = 14;
constexpr u32 POSIX_SC_LINE_MAX = 15;
constexpr u32 POSIX_SC_RE_DUP_MAX = 16;
constexpr u32 POSIX_SC_2_VERSION = 17;
constexpr u32 POSIX_SC_2_C_BIND = 18;
constexpr u32 POSIX_SC_2_C_DEV = 19;
constexpr u32 POSIX_SC_2_CHAR_TERM = 20;
constexpr u32 POSIX_SC_2_FORT_DEV = 21;
constexpr u32 POSIX_SC_2_FORT_RUN = 22;
constexpr u32 POSIX_SC_2_LOCALEDEF = 23;
constexpr u32 POSIX_SC_2_SW_DEV = 24;
constexpr u32 POSIX_SC_2_UPE = 25;
constexpr u32 POSIX_SC_STREAM_MAX = 26;
constexpr u32 POSIX_SC_TZNAME_MAX = 27;
constexpr u32 POSIX_SC_ASYNCHRONOUS_IO = 28;
constexpr u32 POSIX_SC_MAPPED_FILES = 29;
constexpr u32 POSIX_SC_MEMLOCK = 30;
constexpr u32 POSIX_SC_MEMLOCK_RANGE = 31;
constexpr u32 POSIX_SC_MEMORY_PROTECTION = 32;
constexpr u32 POSIX_SC_MESSAGE_PASSING = 33;
constexpr u32 POSIX_SC_PRIORITIZED_IO = 34;
constexpr u32 POSIX_SC_PRIORITY_SCHEDULING = 35;
constexpr u32 POSIX_SC_REALTIME_SIGNALS = 36;
constexpr u32 POSIX_SC_SEMAPHORES = 37;
constexpr u32 POSIX_SC_FSYNC = 38;
constexpr u32 POSIX_SC_SHARED_MEMORY_OBJECTS = 39;
constexpr u32 POSIX_SC_SYNCHRONIZED_IO = 40;
constexpr u32 POSIX_SC_TIMERS = 41;
constexpr u32 POSIX_SC_AIO_LISTIO_MAX = 42;
constexpr u32 POSIX_SC_AIO_MAX = 43;
constexpr u32 POSIX_SC_AIO_PRIO_DELTA_MAX = 44;
constexpr u32 POSIX_SC_DELAYTIMER_MAX = 45;
constexpr u32 POSIX_SC_MQ_OPEN_MAX = 46;
constexpr u32 POSIX_SC_PAGESIZE = 47;
constexpr u32 POSIX_SC_RTSIG_MAX = 48;
constexpr u32 POSIX_SC_SEM_NSEMS_MAX = 49;
constexpr u32 POSIX_SC_SEM_VALUE_MAX = 50;
constexpr u32 POSIX_SC_SIGQUEUE_MAX = 51;
constexpr u32 POSIX_SC_TIMER_MAX = 52;
constexpr u32 POSIX_SC_2_PBS = 59;
constexpr u32 POSIX_SC_2_PBS_ACCOUNTING = 60;
constexpr u32 POSIX_SC_2_PBS_CHECKPOINT = 61;
constexpr u32 POSIX_SC_2_PBS_LOCATE = 62;
constexpr u32 POSIX_SC_2_PBS_MESSAGE = 63;
constexpr u32 POSIX_SC_2_PBS_TRACK = 64;
constexpr u32 POSIX_SC_ADVISORY_INFO = 65;
constexpr u32 POSIX_SC_BARRIERS = 66;
constexpr u32 POSIX_SC_CLOCK_SELECTION = 67;
constexpr u32 POSIX_SC_CPUTIME = 68;
constexpr u32 POSIX_SC_FILE_LOCKING = 69;
constexpr u32 POSIX_SC_GETGR_R_SIZE_MAX = 70;
constexpr u32 POSIX_SC_GETPW_R_SIZE_MAX = 71;
constexpr u32 POSIX_SC_HOST_NAME_MAX = 72;
constexpr u32 POSIX_SC_LOGIN_NAME_MAX = 73;
constexpr u32 POSIX_SC_MONOTONIC_CLOCK = 74;
constexpr u32 POSIX_SC_MQ_PRIO_MAX = 75;
constexpr u32 POSIX_SC_READER_WRITER_LOCKS = 76;
constexpr u32 POSIX_SC_REGEXP = 77;
constexpr u32 POSIX_SC_SHELL = 78;
constexpr u32 POSIX_SC_SPAWN = 79;
constexpr u32 POSIX_SC_SPIN_LOCKS = 80;
constexpr u32 POSIX_SC_SPORADIC_SERVER = 81;
constexpr u32 POSIX_SC_THREAD_ATTR_STACKADDR = 82;
constexpr u32 POSIX_SC_THREAD_ATTR_STACKSIZE = 83;
constexpr u32 POSIX_SC_THREAD_CPUTIME = 84;
constexpr u32 POSIX_SC_THREAD_DESTRUCTOR_ITERATIONS = 85;
constexpr u32 POSIX_SC_THREAD_KEYS_MAX = 86;
constexpr u32 POSIX_SC_THREAD_PRIO_INHERIT = 87;
constexpr u32 POSIX_SC_THREAD_PRIO_PROTECT = 88;
constexpr u32 POSIX_SC_THREAD_PRIORITY_SCHEDULING = 89;
constexpr u32 POSIX_SC_THREAD_PROCESS_SHARED = 90;
constexpr u32 POSIX_SC_THREAD_SAFE_FUNCTIONS = 91;
constexpr u32 POSIX_SC_THREAD_SPORADIC_SERVER = 92;
constexpr u32 POSIX_SC_THREAD_STACK_MIN = 93;
constexpr u32 POSIX_SC_THREAD_THREADS_MAX = 94;
constexpr u32 POSIX_SC_TIMEOUTS = 95;
constexpr u32 POSIX_SC_THREADS = 96;
constexpr u32 POSIX_SC_TRACE = 97;
constexpr u32 POSIX_SC_TRACE_EVENT_FILTER = 98;
constexpr u32 POSIX_SC_TRACE_INHERIT = 99;
constexpr u32 POSIX_SC_TRACE_LOG = 100;
constexpr u32 POSIX_SC_TTY_NAME_MAX = 101;
constexpr u32 POSIX_SC_TYPED_MEMORY_OBJECTS = 102;
constexpr u32 POSIX_SC_V6_ILP32_OFF32 = 103;
constexpr u32 POSIX_SC_V6_ILP32_OFFBIG = 104;
constexpr u32 POSIX_SC_V6_LP64_OFF64 = 105;
constexpr u32 POSIX_SC_V6_LPBIG_OFFBIG = 106;
constexpr u32 POSIX_SC_IPV6 = 118;
constexpr u32 POSIX_SC_RAW_SOCKETS = 119;
constexpr u32 POSIX_SC_SYMLOOP_MAX = 120;
constexpr u32 POSIX_SC_ATEXIT_MAX = 107;
constexpr u32 POSIX_SC_IOV_MAX = 56;
constexpr u32 POSIX_SC_XOPEN_CRYPT = 108;
constexpr u32 POSIX_SC_XOPEN_ENH_I18N = 109;
constexpr u32 POSIX_SC_XOPEN_LEGACY = 110;
constexpr u32 POSIX_SC_XOPEN_REALTIME = 111;
constexpr u32 POSIX_SC_XOPEN_REALTIME_THREADS = 112;
constexpr u32 POSIX_SC_XOPEN_SHM = 113;
constexpr u32 POSIX_SC_XOPEN_STREAMS = 114;
constexpr u32 POSIX_SC_XOPEN_UNIX = 115;
constexpr u32 POSIX_SC_XOPEN_VERSION = 116;
constexpr u32 POSIX_SC_XOPEN_XCU_VERSION = 117;
constexpr u32 POSIX_SC_NPROCESSORS_CONF = 57;
constexpr u32 POSIX_SC_NPROCESSORS_ONLN = 58;
constexpr u32 POSIX_SC_CPUSET_SIZE = 122;
constexpr u32 POSIX_SC_UEXTERR_MAXLEN = 123;
constexpr u32 POSIX_SC_NSIG = 124;
constexpr u32 POSIX_SC_PHYS_PAGES = 121;

} // namespace Libraries::Kernel
