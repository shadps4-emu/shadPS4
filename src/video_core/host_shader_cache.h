// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "common/types.h"

namespace Storage {

class HostShaderCache {
public:
    explicit HostShaderCache(std::filesystem::path root_path);

    std::optional<std::vector<u32>> Load(std::string_view shader_name, std::string_view generation,
                                         std::string_view permutation);
    bool Store(std::string_view shader_name, std::string_view generation,
               std::string_view permutation, std::span<const u32> spirv);

private:
    std::optional<std::filesystem::path> PrepareGenerationLocked(std::string_view shader_name,
                                                                 std::string_view generation);

    std::filesystem::path root;
    std::mutex mutex;
    std::unordered_map<std::string, std::string> prepared_generations;
};

} // namespace Storage
