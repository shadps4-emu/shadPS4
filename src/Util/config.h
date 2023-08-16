#pragma once
#include <filesystem>
#include <types.h>

namespace Config {
void load(const std::filesystem::path& path);
void save(const std::filesystem::path& path);

bool isNeoMode();
u32 getLogLevel();

u32 getScreenWidth();
u32 getScreenHeight();

};  // namespace Config