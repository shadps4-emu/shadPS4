// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/error_codes.h"

constexpr s32 ORBIS_COREDUMP_ERROR_PARAM = 0x81180000;
constexpr s32 ORBIS_COREDUMP_ERROR_NOT_REGISTERED = 0x81180001;
constexpr s32 ORBIS_COREDUMP_ERROR_ALREADY_REGISTERED = 0x81180002;
constexpr s32 ORBIS_COREDUMP_ERROR_NOT_IN_COREDUMP_HANDLER = 0x81180003;
constexpr s32 ORBIS_COREDUMP_ERROR_THREAD_CREATE = 0x81180004;
constexpr s32 ORBIS_COREDUMP_ERROR_STOP_INFO_UNAVAILABLE = 0x81180005;