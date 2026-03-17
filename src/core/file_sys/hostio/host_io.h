// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

#pragma once

#include "host_io_posix.h"
#include "host_io_virtual.h"
#include "host_io_win32.h"
#include "src/host_io_base.h"

using HostIOBase = HostIODriver::HostIO_Base;

using HostVIO = HostIODriver::HostIO_Virtual;

// native implementation
#if defined(__linux__) || defined(__APPLE_CC__)
using HostIO = HostIODriver::HostIO_POSIX;
#elif _WIN32
using HostIO = HostIODriver::HostIO_Win32;
#else
#warning This architecture isn't supported by HostIO
#endif
