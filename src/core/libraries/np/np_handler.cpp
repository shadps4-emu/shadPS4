// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_score/np_score.h"
#include "core/user_settings.h"
#include "np_handler.h"

namespace Libraries::Np {

NpHandler& NpHandler::GetInstance() {
    static NpHandler s_instance;
    return s_instance;
}

bool NpHandler::IsPsnSignedIn(s32 user_id) const {
    if (EmulatorSettings.IsShadNetEnabled()) {
        return true; // fake just return true if shadnet is enabled //TODO
    }
    return false;
}

s32 NpHandler::GetUserIdByOnlineId(const OrbisNpOnlineId& online_id) const {
    return 1000; // return dummy user
}

} // namespace Libraries::Np