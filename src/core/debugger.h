//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Core::Debugger {

bool IsDebuggerAttached();

void WaitForDebuggerAttach();

int GetCurrentPid();

void WaitForPid(int pid);

} // namespace Core::Debugger