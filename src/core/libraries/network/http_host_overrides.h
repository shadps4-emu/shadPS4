// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <string_view>

namespace Libraries::Http std::filesystem::path HostOverridesDir();
bool IsValidHostOverridesTitleId(std::string_view title_id);
void UpdateHostOverridesFromRepo(std::string_view title_id);

} // namespace Libraries::Http
