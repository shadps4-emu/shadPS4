#pragma once
#include <filesystem>

namespace Config {
void load(const std::filesystem::path& path);
void save(const std::filesystem::path& path);

bool isNeoMode();
};  // namespace Config