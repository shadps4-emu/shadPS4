// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "emulator.h"
#include "signals.h"

namespace Core {

Emulator::Emulator() {}
Emulator::~Emulator() {}
void Emulator::Shutdown() {}

SignalDispatch::SignalDispatch() {}
SignalDispatch::~SignalDispatch() {}
void SignalDispatch::RemoveHandlers() {}
bool SignalDispatch::DispatchAccessViolation(void* context, void* fault_address) const {
    return false;
}
bool SignalDispatch::DispatchIllegalInstruction(void* context) const {
    return false;
}

} // namespace Core
