// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>
#include <string_view>
#include <fmt/format.h>
#include "common/logging/backend.h"
#include "common/logging/filter.h"
#include "common/logging/log.h"

namespace Common::Log {

	void FmtLogMessageImpl(Class log_class, Level log_level, const char* filename,
		unsigned int line_num, const char* function, const char* format,
		const fmt::format_args& args) {
	}

	void Initialize(std::string_view) {}
	bool IsActive() { return false; }
	void SetGlobalFilter(const Filter&) {}
	void SetColorConsoleBackendEnabled(bool) {}
	void Start() {}
	void Stop() {}
	void Denitializer() {}
	void SetAppend() {}

} // namespace Common::Log
