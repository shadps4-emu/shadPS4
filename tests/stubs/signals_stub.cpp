// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/signals.h"

namespace Core {

SignalDispatch::SignalDispatch() = default;

SignalDispatch::~SignalDispatch() = default;

void SignalDispatch::RemoveHandlers() {
    // No OS handlers are installed in tests, so there is nothing to remove.
}

bool SignalDispatch::DispatchAccessViolation(void* /*context*/, void* /*fault_address*/) const {
    return false;
}

bool SignalDispatch::DispatchIllegalInstruction(void* /*context*/) const {
    return false;
}

} // namespace Core
