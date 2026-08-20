// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/signals.h"
#include "emulator.h"

namespace Core {

Emulator::Emulator() {}
Emulator::~Emulator() {}
void Emulator::Shutdown() {}

SignalDispatch::SignalDispatch() {}
SignalDispatch::~SignalDispatch() {}
void SignalDispatch::RemoveHandlers() {}

} // namespace Core
