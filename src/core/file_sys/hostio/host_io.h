// INAA License @marecl 2025

#pragma once

#include "host_io_posix.h"
#include "host_io_virtual.h"
#include "host_io_win32.h"
#include "src/host_io_base.h"

using HostIOBase = HostIODriver::HostIO_Base;

using HostVIO = HostIODriver::HostIO_Virtual;

// native implementation
#ifdef __linux__
using HostIO = HostIODriver::HostIO_POSIX;
#elif _WIN32
// #error Contributors needed
using HostIO = HostIODriver::HostIO_Win32;
#else
#warning This architecture isn't supported by HostIO
#endif
