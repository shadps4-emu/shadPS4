// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/asio/io_context.hpp>
#include <condition_variable>
#include <thread>

#include "common/types.h"
#include "core/entry_params.h"
#include "core/libraries/kernel/aio.h"
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

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

void ErrSceToPosix(s32 result);
s32 ErrnoToSceKernelError(s32 e);
void SetPosixErrno(s32 e);
s32* PS4_SYSV_ABI __Error();
const char* PS4_SYSV_ABI sceKernelGetFsSandboxRandomWord();

struct Engine {
    FileSystemEngine m_file_system_engine;
    TimeEngine m_time_engine;
    ThreadsEngine m_threads_engine;
    KernelEventFlagEngine m_kernel_event_flag_engine;
    MemoryEngine m_memory_engine;
    EventQueueEngine m_event_queue_engine;
    ProcessEngine m_process_engine;
    ExceptionEngine m_exception_engine;
    AioEngine m_aio_engine;
    DebugEngine m_debug_engine;

    Engine(Core::Loader::SymbolsResolver* sym);
    ~Engine();

    static constexpr u64 g_stack_chk_guard = 0xDEADBEEF54321ABC; // dummy return

    boost::asio::io_context io_context;
    std::mutex m_asio_req;
    std::condition_variable_any cv_asio_req;
    std::atomic<u32> asio_requests;
    std::jthread service_thread;

    Core::EntryParams entry_params{};

    void KernelServiceThread(const std::stop_token& stoken);

    void KernelSignalRequest();
};


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

} // namespace Libraries
