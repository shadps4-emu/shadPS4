// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef _WIN32

#include <windows.h>
#include <winerror.h>

#include "common/assert.h"
#include "core/libraries/kernel/posix_error.h"

// Convert linux/unix errno to FreeBSD errno
// They differ in higher errno numbers, which may throw Orbis off quite a bit

s32 win2bsd(s32 id) {
    switch (id) {
    default:
        return EIO;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        return ENOENT;
    case ERROR_ALREADY_EXISTS:
        return EEXIST;
    case ERROR_ACCESS_DENIED:
        return EACCES;
    }
}

#endif