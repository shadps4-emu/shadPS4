// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/network/http_host_overrides.h"

#include <cctype>
#include <mutex>
#include <string>
#include "common/path_util.h"

namespace Libraries::Http {

namespace {
std::mutex g_title_id_mutex;
std::string g_title_id;
} // namespace

// Set by the emulator at boot. Kept here instead of reading Common::ElfInfo, whose
// accessors assert when no game is loaded (e.g. in the HTTP unit tests).
void SetHostOverridesTitleId(std::string_view title_id) {
    std::scoped_lock lock{g_title_id_mutex};
    g_title_id = title_id;
}

std::string GetHostOverridesTitleId() {
    std::scoped_lock lock{g_title_id_mutex};
    return g_title_id;
}

std::filesystem::path HostOverridesDir() {
    return Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "host_overrides";
}

bool IsValidHostOverridesTitleId(std::string_view title_id) {
    if (title_id.size() != 9) {
        return false;
    }
    for (std::size_t i = 0; i < 4; ++i) {
        if (!std::isupper(static_cast<unsigned char>(title_id[i]))) {
            return false;
        }
    }
    for (std::size_t i = 4; i < 9; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(title_id[i]))) {
            return false;
        }
    }
    return true;
}

} // namespace Libraries::Http
