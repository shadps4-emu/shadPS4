// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "net.h"

namespace Libraries::Net {

int sys_connect(OrbisNetId s, OrbisNetSockaddr* addr, u32 addrlen);
int sys_bind(OrbisNetId s, OrbisNetSockaddr* addr, u32 addrlen);

} // namespace Libraries::Net