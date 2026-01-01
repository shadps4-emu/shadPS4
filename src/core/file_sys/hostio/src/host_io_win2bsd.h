// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef _WIN32

#include <windows.h>
#include <winerror.h>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/kernel/posix_error.h"

// Convert linux/unix errno to FreeBSD errno
// They differ in higher errno numbers, which may throw Orbis off quite a bit

s32 win2bsd(s32 id) {
    switch (id) {
    default:
        LOG_CRITICAL(Kernel_Fs, "Can't resolve POSIX errno: {}", id);
        return POSIX_EIO;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        return POSIX_ENOENT;
    case ERROR_ALREADY_EXISTS:
        return POSIX_EEXIST;
    case ERROR_ACCESS_DENIED:
        return POSIX_EACCES;
    }
}

#endif