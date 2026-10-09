// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>

namespace Core {

bool StartCpuIdTranslation(int argc, char* argv[], std::string_view mode, bool run_guest = true);

} // namespace Core
