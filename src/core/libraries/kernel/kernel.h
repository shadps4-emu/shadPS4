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

s32 PS4_SYSV_ABI sceKernelGetSystemSwVersion(SwVersionStruct* ret);

struct AuthInfoData {
    u64 paid;
    u64 caps[4];
    u64 attrs[4];
    u64 ucred[8];
};

enum OrbisKernelTitleWorkaroundBits {
    BUG107292_EXTRA_USB_AUDIO_DEVICE = 0x0,
    BUG119504_SUSPEND_BLACK_LIST = 0x1,
    BUG113237_LIVE_DETAIL_BLACK_LIST = 0x2,
    BUG117780_GPU_DOUBLE_PRECISION = 0x3,
    BUG134640_FFXIV_MOVIE_CRASH = 0x4,
    BUG140207_FAKE_BG_EXECUTION = 0x5,
    BUG140207_DELAY_SUSPEND = 0x6,
    BUG141751_FORCE_VIDEO_RECORDING_COMPATIBLE = 0x7,
    BUG141953_FORCE_CONTENT_SEARCH_COMPATIBLE = 0x8,
    BUG135666_FORCED_BASE_MODE = 0x9,
    BUG142996_NEW_QUICK_MENU_BLACK_LIST = 0xa,
    BUG146562_PRODUCT_DETAIL_BLACK_LIST = 0xb,
    BUG141677_DISABLE_SERVICE_ENTITLEMENT_UPDATE_EVENT = 0xc,
    BUG158272_EXTERNAL_HDD_BLACK_LIST = 0xd,
    BUG159526_INVALIDATE_ENTITLEMENT_IN_APPLICATION_DB = 0xe,
    BUG163566_BOOST_MODE_BLACK_LIST = 0xf,
    BUG171584_EXTERNAL_HDD_ACCESS_LATENCY = 0x10,
    BUG183465_SPECIAL_ISSUE = 0x11,
    BUG183542_NEO_VDDNB_VID_3STEP = 0x12,
    BUG183542_NEO_VDDNB_VID_4STEP = 0x13,
    BUG183542_NEO_VDDNB_VID_5STEP = 0x14,
    BUG184831_NEO_VDDNB_VID_STEP_UP_ALL_TITLE = 0x15,
    BUG180029_SAVE_DATA_MEMORY_TIMEOUT_10SEC = 0x16,
    BUG180341_WEBAPI_NOT_COPY_ERROR_JSON = 0x17,
    BUG180847_USE_RECRYPT_BLOCKS = 0x18,
    BUG182301_NP_MANAGER_KEEP_COMPATIBLE = 0x19,
    BUG182170_OSK = 0x1a,
    BUG188290_NEO_SCLK_DOWN_LEVEL1 = 0x1b,
    BUG188290_NEO_SCLK_DOWN_LEVEL2 = 0x1c,
    BUG188290_NEO_SCLK_DOWN_LEVEL3 = 0x1d,
    BUG187987_NTS_CONNECTHASHTABLE = 0x1e,
    BUG190872_HIDE_4K = 0x1f,
    BUG191849_HDCP_CHECK_APP_ONLY = 0x20,
    BUG193000_USE_OLD_WEB_BROWSER_ENGINE = 0x21,
    BUG186690_IME_DISABLE_REMOTE_PLAY = 0x22,
    BUG196278_IME_DISABLE_REMOTE_PLAY_WITH_DISABLE_CONTROLLER = 0x23,
    BUG196285_IME_PACKED_UPDATE_TEXT = 0x24,
    BUG186690_IME_REMOTE_PLAY_FINISHED_BY_PRESS_ENTER = 0x25,
    BUG196699_SYSMODULE_SWITCH_LIBSSL = 0x26,
    BUG192912_PLAYGO_FULL_MULTISTREAM = 0x27,
    BUG201910_DINO_FRONTIER_DLSYM = 0x28,
    BUG202240_HIKARU_UTADA_SCHED = 0x29,
    BUG198989_ANTHEM_KERNEL_PANIC = 0x2a,
    BUG202952_SSL_CHECK_RECV_PENDING_ALWAYS_TRUE = 0x2b,
    BUG203700_PARTY_ROLLBACK = 0x2c,
    CAMELOT3106_USE_OLD_STYLE_USER_AGENT = 0x2d,
    BUG209289_SESSION_SIGNALING_TERMINATE_ON_LEFT = 0x2e,
    BUG198642_LB_SYNC_RESET_TO_FIX_CURSOR_8000 = 0x2f,
    BUG198642_LB_SYNC_RESET_NOT_TO_FIX_CURSOR = 0x30,
    BUG210925_ENABLE_TLS_BUG_FIX = 0x31,
    BUG211235_ENABLE_PRIORITIZE_DLC = 0x32,
    BUG212056_IGNORE_PRELOAD_PATCH_VERSION = 0x33,
    PPRBUG57571_PUSH_CONTEXT_CALLBACK_PTOA_SPEC = 0x34,
    PPRBUG58630_LIBHTTP_USERAGENT_VERSION = 0x35,
    BUG212932_NOT_TO_FIX_POSIX_SEM = 0x36,
    WAL936_DISABLE_KEYBOARD_EVENT_KEY_AND_CODE = 0x37,
    PPRREQ81061_ENABLE_GB_18030_2022_FONTSET = 0x38,
    PPRREQ103800_AGE_RESTRICTION_CHECK = 0x39,
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
