// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

constexpr int ORBIS_SAVE_DATA_ERROR_PARAMETER = 0x809f0000;
constexpr int ORBIS_SAVE_DATA_ERROR_NOT_FOUND = 0x809f0008; // save data doesn't exist
constexpr int ORBIS_SAVE_DATA_ERROR_EXISTS = 0x809f0007;    // save data directory,same name exists