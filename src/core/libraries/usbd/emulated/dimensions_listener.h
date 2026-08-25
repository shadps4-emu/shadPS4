//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "common/types.h"

namespace Libraries::Usbd {

class DimensionsToypad;

// Loopback TCP listener for the emulated LEGO Dimensions Toypad.
//
// The Dimensions Manager dialog is the only way to place a figure, which means
// reaching for a mouse every time a game asks for a different character. This
// listener lets an external companion app do it instead: it accepts the same
// small protocol the Cemu and RPCS3 Toypad forks already speak, so an app
// written for either of those works here unchanged.
//
// It binds 127.0.0.1 only and is disabled unless a port is set in the
// configuration (dimensionsListenerPort, 0 = off). There is no authentication
// and no encryption - loopback-only is the security model.
//
// Wire protocol - every message starts with a 5-byte header:
//
//   [0] command   0x01 LOAD, 0x02 REMOVE, 0x03 MOVE, 0x04 GET_LED
//   [1] pad       1-3 (destination for MOVE)
//   [2] slot      0-6 (destination for MOVE)
//   [3] src pad   MOVE only, otherwise 0
//   [4] src slot  MOVE only, otherwise 0
//
// LOAD then carries 180 raw tag bytes, a u16 little-endian path length and that
// many UTF-8 path bytes. With a path the file is opened read/write so the game's
// writes persist back into the .bin, exactly as a dialog-loaded figure does;
// with a zero-length path the figure lives in memory for the session only.
//
// LOAD, REMOVE and MOVE are fire-and-forget. GET_LED is the only command that
// replies: a 30-byte snapshot of what the game currently has the three LED
// regions doing - see DimensionsToypad's LED mirror.
class DimensionsListener {
public:
    explicit DimensionsListener(std::shared_ptr<DimensionsToypad> toypad);
    ~DimensionsListener();

    DimensionsListener(const DimensionsListener&) = delete;
    DimensionsListener& operator=(const DimensionsListener&) = delete;

    // Binds and starts accepting. A port of 0 means "disabled" and does nothing,
    // so the listener costs exactly one branch when it is not configured.
    void Start(u16 port);

    // Closes the listening socket, which drops the accept loop out of its blocking
    // call, then joins the thread. Safe to call when never started.
    void Stop();

private:
    void Run(u16 port);
    void HandleClient(u64 client);

    std::shared_ptr<DimensionsToypad> m_toypad;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    // Held as u64 so the header does not have to drag in winsock / sys/socket.
    std::atomic<u64> m_listen_socket{~0ULL};
};

} // namespace Libraries::Usbd
