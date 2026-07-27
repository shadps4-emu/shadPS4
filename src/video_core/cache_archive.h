// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>

namespace Storage::Detail {

/// Rewrites an archive with at most one entry per name. If an entry occurs more than once, the
/// last occurrence is retained. The destination is removed when compaction fails.
bool CompactArchive(const std::filesystem::path& source, const std::filesystem::path& destination);

} // namespace Storage::Detail
