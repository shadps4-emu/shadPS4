// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <string>
#include <string_view>
#include <unordered_map>

#include <spdlog/spdlog.h>

#include "common/decoder.h"
#include "common/logging/log.h"
#include "common/rdtsc.h"
#include "common/signal_context.h"
#include "common/thread.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/threads/exception.h"
#include "core/libraries/kernel/threads/pthread.h"

void assert_fail_impl() {
    std::_Exit(1);
}

[[noreturn]] void unreachable_impl() {
    std::_Exit(1);
}

namespace Common {

std::string GetCurrentThreadName() {
    return "shadPS4::PauseIntegrationTest";
}

void* GetRip(void*) {
    return nullptr;
}

bool IsWriteError(void*) {
    return false;
}

u64 EstimateRDTSCFrequency() {
    return 1'000'000'000;
}

DecoderImpl::DecoderImpl() {
    ZydisDecoderInit(&m_decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    ZydisFormatterInit(&m_formatter, ZYDIS_FORMATTER_STYLE_INTEL);
}

DecoderImpl::~DecoderImpl() = default;

std::string DecoderImpl::disassembleInst(ZydisDecodedInstruction&, ZydisDecodedOperand*, u64) {
    return {};
}

ZyanStatus DecoderImpl::decodeInstruction(ZydisDecodedInstruction& inst,
                                          ZydisDecodedOperand* operands, void* data, u64 size) {
    return ZydisDecoderDecodeFull(&m_decoder, data, size, &inst, operands);
}

} // namespace Common

namespace Common::Log {

bool g_should_append{};
std::unordered_map<std::string_view, std::shared_ptr<spdlog::logger>> ALL_LOGGERS{};

} // namespace Common::Log

namespace Libraries::Kernel {

thread_local Pthread* g_curthread{};

s32 NativeToOrbisSignal(const s32 signal) {
    return signal;
}

s32 NativeToPosixErrno(const s32 error) {
    return error;
}

#ifndef _WIN32
Ucontext::Ucontext(siginfo_t const*, ucontext_t* raw_context) : host_context{raw_context} {}
#endif

void Ucontext::SyncHostFromGuest() {}

bool Pthread::DispatchSignal(s32, Siginfo*, Ucontext*) {
    return false;
}

bool Pthread::DispatchPendingSignals(Siginfo*, Ucontext*) {
    return false;
}

} // namespace Libraries::Kernel
