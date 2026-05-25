// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <common/types.h>
#include "np_types.h"

namespace Libraries::Np {

class NpHandler {
public:
    static NpHandler& GetInstance();

    NpHandler(const NpHandler&) = delete;
    NpHandler& operator=(const NpHandler&) = delete;

    // True if this specific user is authenticated to the shadNet server.
    bool IsPsnSignedIn(s32 user_id) const;

    // Reverse lookup: OrbisNpOnlineId to local user_id.
    // Scans m_np_ids for a matching handle.data string.
    // Returns -1 if no connected user has that Online ID.
    s32 GetUserIdByOnlineId(const OrbisNpOnlineId& online_id) const;

private:
    NpHandler() = default;
    ~NpHandler() = default;
};

} // namespace Libraries::Np