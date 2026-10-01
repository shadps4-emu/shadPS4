// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Net::P2P {
class Keyring;
}

namespace Libraries::Net {

// np_port is in network byte order. Returns 0 or a guest errno.
int P2PKeyIoctl(Core::Net::P2P::Keyring& keyring, u16 np_port, u64 cmd, void* data);

} // namespace Libraries::Net
