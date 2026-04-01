//  SPDX-FileCopyrightText: Copyright 2026 shadBloodborne Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Core::Debugger {

bool IsDebuggerAttached();

void WaitForDebuggerAttach();

int GetCurrentPid();

void WaitForPid(int pid);

} // namespace Core::Debugger