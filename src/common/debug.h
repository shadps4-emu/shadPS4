// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef _MSC_VER
#define BREAKPOINT __debugbreak
#elif defined(__GNUC__)
#define BREAKPOINT __builtin_trap
#else
#error What the fuck is this compiler
#endif
