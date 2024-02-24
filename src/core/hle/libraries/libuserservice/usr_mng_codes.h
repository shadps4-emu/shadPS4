// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

constexpr int SCE_USER_SERVICE_MAX_LOGIN_USERS = 4;       // Max users logged in at once
constexpr int SCE_USER_SERVICE_MAX_USER_NAME_LENGTH = 16; // Max length for user name

constexpr int SCE_USER_SERVICE_USER_ID_INVALID = -1; // Invalid user ID
constexpr int SCE_USER_SERVICE_USER_ID_SYSTEM = 255; // Generic id for device
constexpr int SCE_USER_SERVICE_USER_ID_EVERYONE =
    254; // Generic id for user (mostly used in common dialogs)
