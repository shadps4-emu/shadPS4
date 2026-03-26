// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string>
#include "common/scm_rev.h"

namespace Common {

	constexpr char g_version[] = "0.0.0 TEST";
	constexpr bool g_is_release = false;
	constexpr char g_scm_rev[] = "test_rev_hash";
	constexpr char g_scm_branch[] = "test_branch";
	constexpr char g_scm_desc[] = "test_desc";
	constexpr char g_scm_remote_name[] = "origin";
	constexpr char g_scm_remote_url[] = "https://github.com/test/shadPS4";
	constexpr char g_scm_date[] = "2026-03-23";

	const std::string GetRemoteNameFromLink() {
		return "test";
	}

} // namespace Common
