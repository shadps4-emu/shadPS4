// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/error_codes.h"

// For error codes shared between multiple Np libraries.
constexpr int ORBIS_NP_ERROR_INVALID_ARGUMENT = 0x80550003;
constexpr int ORBIS_NP_ERROR_SIGNED_OUT = 0x80550006;
constexpr int ORBIS_NP_ERROR_USER_NOT_FOUND = 0x80550007;
constexpr int ORBIS_NP_ERROR_INVALID_SIZE = 0x80550011;
constexpr int ORBIS_NP_ERROR_ABORTED = 0x80550012;
constexpr int ORBIS_NP_ERROR_REQUEST_MAX = 0x80550013;
constexpr int ORBIS_NP_ERROR_REQUEST_NOT_FOUND = 0x80550014;
constexpr int ORBIS_NP_ERROR_INVALID_ID = 0x80550015;