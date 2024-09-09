// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <set>
#include "common/singleton.h"

namespace Core {

using AccessViolationHandler = bool (*)(void* code_address, void* fault_address, bool is_write);
using IllegalInstructionHandler = bool (*)(void* code_address);

/// Receives OS signals and dispatches to the appropriate handlers.
class SignalDispatch {
public:
    SignalDispatch();
    ~SignalDispatch();

    /// Registers a handler for memory access violation signals.
    void RegisterAccessViolationHandler(const AccessViolationHandler& handler, u32 priority) {
        access_violation_handlers.emplace(handler, priority);
    }

    /// Registers a handler for illegal instruction signals.
    void RegisterIllegalInstructionHandler(const IllegalInstructionHandler& handler, u32 priority) {
        illegal_instruction_handlers.emplace(handler, priority);
    }

    /// Dispatches an access violation signal, returning whether it was successfully handled.
    bool DispatchAccessViolation(void* code_address, void* fault_address, bool is_write) const;

    /// Dispatches an illegal instruction signal, returning whether it was successfully handled.
    bool DispatchIllegalInstruction(void* code_address) const;

private:
    template <typename T>
    struct HandlerEntry {
        T handler;
        u32 priority;

        std::strong_ordering operator<=>(const HandlerEntry& right) const {
            return priority <=> right.priority;
        }
    };
    std::set<HandlerEntry<AccessViolationHandler>> access_violation_handlers;
    std::set<HandlerEntry<IllegalInstructionHandler>> illegal_instruction_handlers;

#ifdef _WIN32
    void* handle{};
#endif
};

using Signals = Common::Singleton<SignalDispatch>;

} // namespace Core