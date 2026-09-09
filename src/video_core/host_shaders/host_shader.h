// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>

namespace HostShaders {

struct ShaderSource {
    std::string_view name;
    std::string_view code;
};

} // namespace HostShaders
