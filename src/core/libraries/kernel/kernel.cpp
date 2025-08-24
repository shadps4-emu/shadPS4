// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
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
#include "core/libraries/kernel/time.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/sys_net.h"

#ifdef _WIN64
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif
#include <common/singleton.h>
#include <core/libraries/network/net_error.h>
#include <core/libraries/network/sockets.h>
#include "aio.h"

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

s32 PS4_SYSV_ABI sceKernelGetSystemSwVersion(SwVersionStruct* ret) {
    if (ret == nullptr) {
        return ORBIS_OK; // but why?
    }
    ASSERT(ret->struct_size == 40);
    u32 fake_fw = Common::ElfInfo::Instance().RawFirmwareVer();
    ret->hex_representation = fake_fw;
    std::snprintf(ret->text_representation, 28, "%2x.%03x.%03x", fake_fw >> 0x18,
                  fake_fw >> 0xc & 0xfff, fake_fw & 0xfff); // why %2x?
    LOG_INFO(Lib_Kernel, "called, returned sw version: {}", ret->text_representation);
    return ORBIS_OK;
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

    LIB_OBJ("f7uOxY9mM1U", "libkernel", 1, "libkernel", 1, 1, &g_stack_chk_guard);
    LIB_FUNCTION("D4yla3vx4tY", "libkernel", 1, "libkernel", 1, 1, sceKernelError);
    LIB_FUNCTION("Mv1zUObHvXI", "libkernel", 1, "libkernel", 1, 1, sceKernelGetSystemSwVersion);
    LIB_FUNCTION("PfccT7qURYE", "libkernel", 1, "libkernel", 1, 1, kernel_ioctl);
    LIB_FUNCTION("JGfTMBOdUJo", "libkernel", 1, "libkernel", 1, 1, sceKernelGetFsSandboxRandomWord);
    LIB_FUNCTION("6xVpy0Fdq+I", "libkernel", 1, "libkernel", 1, 1, _sigprocmask);
    LIB_FUNCTION("Xjoosiw+XPI", "libkernel", 1, "libkernel", 1, 1, sceKernelUuidCreate);
    LIB_FUNCTION("Ou3iL1abvng", "libkernel", 1, "libkernel", 1, 1, stack_chk_fail);
    LIB_FUNCTION("9BcDykPmo1I", "libkernel", 1, "libkernel", 1, 1, __Error);
    LIB_FUNCTION("k+AXqu2-eBc", "libkernel", 1, "libkernel", 1, 1, posix_getpagesize);
    LIB_FUNCTION("k+AXqu2-eBc", "libScePosix", 1, "libkernel", 1, 1, posix_getpagesize);
    LIB_FUNCTION("NWtTN10cJzE", "libSceLibcInternalExt", 1, "libSceLibcInternal", 1, 1,
                 sceLibcHeapGetTraceInfo);

    // network
    LIB_FUNCTION("XVL8So3QJUk", "libkernel", 1, "libkernel", 1, 1, Libraries::Net::sys_connect);
    LIB_FUNCTION("pG70GT5yRo4", "libkernel", 1, "libkernel", 1, 1, Libraries::Net::sys_socketex);
    LIB_FUNCTION("KuOmgKoqCdY", "libkernel", 1, "libkernel", 1, 1, Libraries::Net::sys_bind);
    LIB_FUNCTION("pxnCmagrtao", "libkernel", 1, "libkernel", 1, 1, Libraries::Net::sys_listen);
    LIB_FUNCTION("3e+4Iv7IJ8U", "libkernel", 1, "libkernel", 1, 1, Libraries::Net::sys_accept);
    LIB_FUNCTION("TU-d9PfIHPM", "libkernel", 1, "libkernel", 1, 1, Libraries::Net::sys_socket);
    LIB_FUNCTION("K1S8oc61xiM", "libkernel", 1, "libkernel", 1, 1, Libraries::Net::sys_htonl);
    LIB_FUNCTION("jogUIsOV3-U", "libkernel", 1, "libkernel", 1, 1, Libraries::Net::sys_htons);
    LIB_FUNCTION("oBr313PppNE", "libkernel", 1, "libkernel", 1, 1, Libraries::Net::sys_sendto);
    LIB_FUNCTION("lUk6wrGXyMw", "libkernel", 1, "libkernel", 1, 1, Libraries::Net::sys_recvfrom);

    LIB_FUNCTION("TU-d9PfIHPM", "libScePosix", 1, "libkernel", 1, 1, Libraries::Net::sys_socket);
    LIB_FUNCTION("oBr313PppNE", "libScePosix", 1, "libkernel", 1, 1, Libraries::Net::sys_sendto);
    LIB_FUNCTION("lUk6wrGXyMw", "libScePosix", 1, "libkernel", 1, 1, Libraries::Net::sys_recvfrom);
    LIB_FUNCTION("TXFFFiNldU8", "libScePosix", 1, "libkernel", 1, 1,
                 Libraries::Net::sys_getpeername);
    LIB_FUNCTION("6O8EwYOgH9Y", "libScePosix", 1, "libkernel", 1, 1,
                 Libraries::Net::sys_getsockopt);
    LIB_FUNCTION("fFxGkxF2bVo", "libScePosix", 1, "libkernel", 1, 1,
                 Libraries::Net::sys_setsockopt);
    LIB_FUNCTION("RenI1lL1WFk", "libScePosix", 1, "libkernel", 1, 1,
                 Libraries::Net::sys_getsockname);
    LIB_FUNCTION("KuOmgKoqCdY", "libScePosix", 1, "libkernel", 1, 1, Libraries::Net::sys_bind);
    LIB_FUNCTION("5jRCs2axtr4", "libScePosix", 1, "libkernel", 1, 1,
                 Libraries::Net::sceNetInetNtop); // TODO fix it to sys_ ...
    LIB_FUNCTION("4n51s0zEf0c", "libScePosix", 1, "libkernel", 1, 1,
                 Libraries::Net::sceNetInetPton); // TODO fix it to sys_ ...
    LIB_FUNCTION("XVL8So3QJUk", "libScePosix", 1, "libkernel", 1, 1, Libraries::Net::sys_connect);
    LIB_FUNCTION("3e+4Iv7IJ8U", "libScePosix", 1, "libkernel", 1, 1, Libraries::Net::sys_accept);
    LIB_FUNCTION("aNeavPDNKzA", "libScePosix", 1, "libkernel", 1, 1, Libraries::Net::sys_sendmsg);
    LIB_FUNCTION("pxnCmagrtao", "libScePosix", 1, "libkernel", 1, 1, Libraries::Net::sys_listen);

    LIB_FUNCTION("4oXYe9Xmk0Q", "libkernel", 1, "libkernel", 1, 1, sceKernelGetGPI);
    LIB_FUNCTION("ca7v6Cxulzs", "libkernel", 1, "libkernel", 1, 1, sceKernelSetGPO);
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
    case ESRCH:
        g_posix_errno = POSIX_ESRCH;
        break;
    case EINTR:
        g_posix_errno = POSIX_EINTR;
        break;
    case EIO:
        g_posix_errno = POSIX_EIO;
        break;
    case ENXIO:
        g_posix_errno = POSIX_ENXIO;
        break;
    case E2BIG:
        g_posix_errno = POSIX_E2BIG;
        break;
    case ENOEXEC:
        g_posix_errno = POSIX_ENOEXEC;
        break;
    case EBADF:
        g_posix_errno = POSIX_EBADF;
        break;
    case ECHILD:
        g_posix_errno = POSIX_ECHILD;
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
    case ENOTBLK:
        g_posix_errno = POSIX_ENOTBLK;
        break;
    case EBUSY:
        g_posix_errno = POSIX_EBUSY;
        break;
    case EEXIST:
        g_posix_errno = POSIX_EEXIST;
        break;
    case EXDEV:
        g_posix_errno = POSIX_EXDEV;
        break;
    case ENODEV:
        g_posix_errno = POSIX_ENODEV;
        break;
    case ENOTDIR:
        g_posix_errno = POSIX_ENOTDIR;
        break;
    case EISDIR:
        g_posix_errno = POSIX_EISDIR;
        break;
    case EINVAL:
        g_posix_errno = POSIX_EINVAL;
        break;
    case ENFILE:
        g_posix_errno = POSIX_ENFILE;
        break;
    case EMFILE:
        g_posix_errno = POSIX_EMFILE;
        break;
    case ENOTTY:
        g_posix_errno = POSIX_ENOTTY;
        break;
    case ETXTBSY:
        g_posix_errno = POSIX_ETXTBSY;
        break;
    case EFBIG:
        g_posix_errno = POSIX_EFBIG;
        break;
    case ENOSPC:
        g_posix_errno = POSIX_ENOSPC;
        break;
    case ESPIPE:
        g_posix_errno = POSIX_ESPIPE;
        break;
    case EROFS:
        g_posix_errno = POSIX_EROFS;
        break;
    case EMLINK:
        g_posix_errno = POSIX_EMLINK;
        break;
    case EPIPE:
        g_posix_errno = POSIX_EPIPE;
        break;
    case EDOM:
        g_posix_errno = POSIX_EDOM;
        break;
    case ERANGE:
        g_posix_errno = POSIX_ERANGE;
        break;
    case EAGAIN:                      // same as EWOULDBLOCK
        g_posix_errno = POSIX_EAGAIN; // same as POSIX_EWOULDBLOCK
        break;
    case EINPROGRESS:
        g_posix_errno = POSIX_EINPROGRESS;
        break;
    case EALREADY:
        g_posix_errno = POSIX_EALREADY;
        break;
    case ENOTSOCK:
        g_posix_errno = POSIX_ENOTSOCK;
        break;
    case EDESTADDRREQ:
        g_posix_errno = POSIX_EDESTADDRREQ;
        break;
    case EMSGSIZE:
        g_posix_errno = POSIX_EMSGSIZE;
        break;
    case EPROTOTYPE:
        g_posix_errno = POSIX_EPROTOTYPE;
        break;
    case ENOPROTOOPT:
        g_posix_errno = POSIX_ENOPROTOOPT;
        break;
    case EPROTONOSUPPORT:
        g_posix_errno = POSIX_EPROTONOSUPPORT;
        break;
    case ESOCKTNOSUPPORT:
        g_posix_errno = POSIX_ESOCKTNOSUPPORT;
        break;
    case EOPNOTSUPP:                      // same as ENOTSUP
        g_posix_errno = POSIX_EOPNOTSUPP; // same as POSIX_ENOTSUP
        break;
    case EPFNOSUPPORT:
        g_posix_errno = POSIX_EPFNOSUPPORT;
        break;
    case EAFNOSUPPORT:
        g_posix_errno = POSIX_EAFNOSUPPORT;
        break;
    case EADDRINUSE:
        g_posix_errno = POSIX_EADDRINUSE;
        break;
    case EADDRNOTAVAIL:
        g_posix_errno = POSIX_EADDRNOTAVAIL;
        break;
    case ENETDOWN:
        g_posix_errno = POSIX_ENETDOWN;
        break;
    case ENETUNREACH:
        g_posix_errno = POSIX_ENETUNREACH;
        break;
    case ENETRESET:
        g_posix_errno = POSIX_ENETRESET;
        break;
    case ECONNABORTED:
        g_posix_errno = POSIX_ECONNABORTED;
        break;
    case ECONNRESET:
        g_posix_errno = POSIX_ECONNRESET;
        break;
    case ENOBUFS:
        g_posix_errno = POSIX_ENOBUFS;
        break;
    case EISCONN:
        g_posix_errno = POSIX_EISCONN;
        break;
    case ENOTCONN:
        g_posix_errno = POSIX_ENOTCONN;
        break;
    case ESHUTDOWN:
        g_posix_errno = POSIX_ESHUTDOWN;
        break;
    case ETOOMANYREFS:
        g_posix_errno = POSIX_ETOOMANYREFS;
        break;
    case ETIMEDOUT:
        g_posix_errno = POSIX_ETIMEDOUT;
        break;
    case ECONNREFUSED:
        g_posix_errno = POSIX_ECONNREFUSED;
        break;
    case ELOOP:
        g_posix_errno = POSIX_ELOOP;
        break;
    case ENAMETOOLONG:
        g_posix_errno = POSIX_ENAMETOOLONG;
        break;
    case EHOSTDOWN:
        g_posix_errno = POSIX_EHOSTDOWN;
        break;
    case EHOSTUNREACH:
        g_posix_errno = POSIX_EHOSTUNREACH;
        break;
    case ENOTEMPTY:
        g_posix_errno = POSIX_ENOTEMPTY;
        break;
    // case EPROCLIM:
    //     g_posix_errno = POSIX_EPROCLIM;
    //     break;
    case EUSERS:
        g_posix_errno = POSIX_EUSERS;
        break;
    case EDQUOT:
        g_posix_errno = POSIX_EDQUOT;
        break;
    case ESTALE:
        g_posix_errno = POSIX_ESTALE;
        break;
    case EREMOTE:
        g_posix_errno = POSIX_EREMOTE;
        break;
    // case EBADRPC:
    //     g_posix_errno = POSIX_EBADRPC;
    //     break;
    // case ERPCMISMATCH:
    //     g_posix_errno = POSIX_ERPCMISMATCH;
    //     break;
    // case EPROGUNAVAIL:
    //     g_posix_errno = POSIX_EPROGUNAVAIL;
    //     break;
    // case EPROGMISMATCH:
    //     g_posix_errno = POSIX_EPROGMISMATCH;
    //     break;
    // case EPROCUNAVAIL:
    //     g_posix_errno = POSIX_EPROCUNAVAIL;
    //     break;
    case ENOLCK:
        g_posix_errno = POSIX_ENOLCK;
        break;
    case ENOSYS:
        g_posix_errno = POSIX_ENOSYS;
        break;
    // case EFTYPE:
    //     g_posix_errno = POSIX_EFTYPE;
    //     break;
    // case EAUTH:
    //     g_posix_errno = POSIX_EAUTH;
    //     break;
    // case ENEEDAUTH:
    //     g_posix_errno = POSIX_ENEEDAUTH;
    //     break;
    case EIDRM:
        g_posix_errno = POSIX_EIDRM;
        break;
    case ENOMSG:
        g_posix_errno = POSIX_ENOMSG;
        break;
    case EOVERFLOW:
        g_posix_errno = POSIX_EOVERFLOW;
        break;
    case ECANCELED:
        g_posix_errno = POSIX_ECANCELED;
        break;
    case EILSEQ:
        g_posix_errno = POSIX_EILSEQ;
        break;
    // case ENOATTR:
    //     g_posix_errno = POSIX_ENOATTR;
    //     break;
    // case EDOOFUS:
    //     g_posix_errno = POSIX_EDOOFUS;
    //     break;
    case EBADMSG:
        g_posix_errno = POSIX_EBADMSG;
        break;
    case EMULTIHOP:
        g_posix_errno = POSIX_EMULTIHOP;
        break;
    case ENOLINK:
        g_posix_errno = POSIX_ENOLINK;
        break;
    case EPROTO:
        g_posix_errno = POSIX_EPROTO;
        break;
    // case ENOTCAPABLE:
    //     g_posix_errno = POSIX_ENOTCAPABLE;
    //     break;
    // case ECAPMODE:
    //     g_posix_errno = POSIX_ECAPMODE;
    //     break;
    // case ENOBLK:
    //     g_posix_errno = POSIX_ENOBLK;
    //     break;
    // case EICV:
    //     g_posix_errno = POSIX_EICV;
    //     break;
    // case ENOPLAYGOENT:
    //     g_posix_errno = POSIX_ENOPLAYGOENT;
    //     break;
    // case EREVOKE:
    //     g_posix_errno = POSIX_EREVOKE;
    //     break;
    // case ESDKVERSION:
    //     g_posix_errno = POSIX_ESDKVERSION;
    //     break;
    // case ESTART:
    //     g_posix_errno = POSIX_ESTART;
    //     break;
    // case ESTOP:
    //     g_posix_errno = POSIX_ESTOP;
    //     break;
    // case EINVALID2MB:
    //     g_posix_errno = POSIX_EINVALID2MB;
    //     break;
    // case ELAST:
    //     g_posix_errno = POSIX_ELAST;
    //     break;
    // case EADHOC:
    //     g_posix_errno = POSIX_EADHOC;
    //     break;
    // case EINACTIVEDISABLED:
    //     g_posix_errno = POSIX_EINACTIVEDISABLED;
    //     break;
    // case ENETNODATA:
    //     g_posix_errno = POSIX_ENETNODATA;
    //     break;
    // case ENETDESC:
    //     g_posix_errno = POSIX_ENETDESC;
    //     break;
    // case ENETDESCTIMEDOUT:
    //     g_posix_errno = POSIX_ENETDESCTIMEDOUT;
    //     break;
    // case ENETINTR:
    //     g_posix_errno = POSIX_ENETINTR;
    //     break;
    // case ERETURN:
    //     g_posix_errno = POSIX_ERETURN;
    //     break;
    // case EFPOS:
    //     g_posix_errno = POSIX_EFPOS;
    //     break;
    case ENODATA:
        g_posix_errno = POSIX_ENODATA;
        break;
    case ENOSR:
        g_posix_errno = POSIX_ENOSR;
        break;
    case ENOSTR:
        g_posix_errno = POSIX_ENOSTR;
        break;
    case ENOTRECOVERABLE:
        g_posix_errno = POSIX_ENOTRECOVERABLE;
        break;
    // case EOTHER:
    //     g_posix_errno = POSIX_EOTHER;
    //     break;
    case EOWNERDEAD:
        g_posix_errno = POSIX_EOWNERDEAD;
        break;
    case ETIME:
        g_posix_errno = POSIX_ETIME;
        break;
    default:
        LOG_WARNING(Kernel, "Unhandled errno {}", e);
        g_posix_errno = e;
    }
}

} // namespace Libraries::Kernel
