// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// net errno codes
constexpr int ORBIS_NET_ENOENT = 2;
constexpr int ORBIS_NET_EBADF = 9;
constexpr int ORBIS_NET_EFAULT = 14;
constexpr int ORBIS_NET_EEXIST = 17;
constexpr int ORBIS_NET_EINVAL = 22;

// error codes
constexpr int ORBIS_NET_ERROR_EPERM = 0x80410101;
constexpr int ORBIS_NET_ERROR_ENOENT = 0x80410102;
constexpr int ORBIS_NET_ERROR_EBADF = 0x80410109;
constexpr int ORBIS_NET_ERROR_EFAULT = 0x8041010e;
constexpr int ORBIS_NET_ERROR_EEXIST = 0x80410111;
constexpr int ORBIS_NET_ERROR_EINVAL = 0x80410116;
constexpr int ORBIS_NET_ERROR_ENOPROTOOPT = 0x8041012a;
