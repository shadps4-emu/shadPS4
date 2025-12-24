// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/types.h"
#include <fmt/format.h>

namespace Common::Log {

void FmtLogMessageImpl(Class log_class, Level log_level, const char *filename,
                       unsigned int line_num, const char *function,
                       const char *format, const fmt::format_args &args) {
  // Stub implementation - just ignore logs in tests
  (void)log_class;
  (void)log_level;
  (void)filename;
  (void)line_num;
  (void)function;
  (void)format;
  (void)args;
}

void Start() {}
void Stop() {}

} // namespace Common::Log
