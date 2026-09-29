// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// sceNetIoctl on P2P sockets. The NP libraries use it to install P2P traffic keys and set the
// communication ID. Ported from the earlier RE'd implementation.
//
// Commands are FreeBSD ioctls in group 'P'. The argument is a P2P sockaddr (16 bytes for IPv4)
// followed by a 20-byte key record:
//   [sockaddr (16)][key (16)][local port (2, BE)][flags (1)][id (1)]
//
//   0xc8  0x200050c8 is a no-op, 0x802450c8/0x803450c8 remove all peer keys with the id
//   0xc9  set default key, also installed for 127.0.0.1:3658
//   0xca  read default key into the key field (zeros if unset)
//   0xcb  add peer key for the sockaddr, or take a ref if it exists
//   0xcc  release peer key, value and id must match, else ENOENT
//   0xcd  set wildcard key
//   0xce  read wildcard key
//   0xcf, 0xd0  write NP UDP port (2 bytes, BE) to the start of the argument
//   0xd8  set communication ID from the first 16 bytes
//   0xd9  clear communication ID

#pragma once

#include "common/types.h"

namespace Core::Net::P2P {
class Keyring;
}

namespace Libraries::Net {

// np_port is in network byte order. Returns 0 or a guest errno.
int P2PKeyIoctl(Core::Net::P2P::Keyring& keyring, u16 np_port, u64 cmd, void* data);

} // namespace Libraries::Net
