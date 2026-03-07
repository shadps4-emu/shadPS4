// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "log.h"

namespace Common::Log {
std::shared_ptr<spdlog_stdout> g_console_sink;
std::shared_ptr<spdlog::sinks::basic_file_sink_mt> g_shad_file_sink;
std::shared_ptr<spdlog::sinks::basic_file_sink_mt> g_game_file_sink;
bool g_should_append = false;
} // namespace Common::Log
