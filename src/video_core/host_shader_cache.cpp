// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <atomic>
#include <chrono>
#include <fstream>
#include <system_error>
#include <type_traits>

#include "common/sha1.h"
#include "video_core/host_shader_cache.h"

namespace Storage {
namespace {

constexpr std::array<u8, 8> CacheMagic{'S', 'H', 'A', 'D', 'H', 'S', 'P', 'V'};
constexpr u32 CacheVersion = 1;
constexpr u32 SpirvMagic = 0x07230203;
constexpr size_t MaxSpirvWords = 16 * 1024 * 1024 / sizeof(u32);

struct CacheHeader {
    std::array<u8, 8> magic;
    u32 version;
    u32 word_count;
    std::array<u8, 20> digest;
};

static_assert(std::is_trivially_copyable_v<CacheHeader>);
static_assert(sizeof(CacheHeader) == 36);

bool IsSafeName(std::string_view value, bool allow_period) {
    if (value.empty()) {
        return false;
    }
    for (const char character : value) {
        const bool alpha_numeric = (character >= 'a' && character <= 'z') ||
                                   (character >= 'A' && character <= 'Z') ||
                                   (character >= '0' && character <= '9');
        if (!alpha_numeric && character != '-' && character != '_' &&
            (!allow_period || character != '.')) {
            return false;
        }
    }
    return true;
}

std::array<u8, 20> Digest(std::span<const u32> data) {
    sha1::SHA1 hash;
    hash.processBytes(data.data(), data.size_bytes());
    std::array<u8, 20> digest{};
    hash.getDigestBytes(digest.data());
    return digest;
}

std::filesystem::path TemporaryPath(const std::filesystem::path& destination) {
    static std::atomic<u64> temporary_id{};
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return destination.string() + ".tmp." + std::to_string(timestamp) + "." +
           std::to_string(temporary_id.fetch_add(1, std::memory_order_relaxed));
}

bool Publish(const std::filesystem::path& source, const std::filesystem::path& destination) {
    std::error_code ec;
    std::filesystem::rename(source, destination, ec);
    if (!ec) {
        return true;
    }

    // A concurrent process may have published the same permutation first.
    if (std::filesystem::is_regular_file(destination, ec)) {
        std::filesystem::remove(source, ec);
        return true;
    }
    std::filesystem::remove(source, ec);
    return false;
}

} // namespace

HostShaderCache::HostShaderCache(std::filesystem::path root_path) : root{std::move(root_path)} {}

std::optional<std::filesystem::path> HostShaderCache::PrepareGenerationLocked(
    std::string_view shader_name, std::string_view generation) {
    if (!IsSafeName(shader_name, true) || !IsSafeName(generation, false)) {
        return std::nullopt;
    }

    const auto shader_path = root / shader_name;
    const auto generation_path = shader_path / generation;
    const auto prepared = prepared_generations.find(std::string{shader_name});
    if (prepared != prepared_generations.end() && prepared->second == generation) {
        return generation_path;
    }

    std::error_code ec;
    std::filesystem::create_directories(shader_path, ec);
    if (ec) {
        return std::nullopt;
    }

    for (std::filesystem::directory_iterator iter{shader_path, ec}, end; !ec && iter != end;
         iter.increment(ec)) {
        if (iter->path().filename() != generation) {
            std::filesystem::remove_all(iter->path(), ec);
            if (ec) {
                return std::nullopt;
            }
        }
    }
    if (ec) {
        return std::nullopt;
    }

    std::filesystem::create_directories(generation_path, ec);
    if (ec) {
        return std::nullopt;
    }
    prepared_generations.insert_or_assign(std::string{shader_name}, std::string{generation});
    return generation_path;
}

std::optional<std::vector<u32>> HostShaderCache::Load(std::string_view shader_name,
                                                      std::string_view generation,
                                                      std::string_view permutation) {
    std::scoped_lock lock{mutex};
    if (!IsSafeName(permutation, false)) {
        return std::nullopt;
    }
    const auto generation_path = PrepareGenerationLocked(shader_name, generation);
    if (!generation_path) {
        return std::nullopt;
    }

    std::ifstream file{*generation_path / (std::string{permutation} + ".spv"),
                       std::ios::binary | std::ios::ate};
    if (!file) {
        return std::nullopt;
    }
    const auto file_size = file.tellg();
    if (file_size < static_cast<std::streamoff>(sizeof(CacheHeader))) {
        return std::nullopt;
    }
    file.seekg(0);

    CacheHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    const auto expected_size = sizeof(CacheHeader) + size_t{header.word_count} * sizeof(u32);
    if (!file || header.magic != CacheMagic || header.version != CacheVersion ||
        header.word_count == 0 || header.word_count > MaxSpirvWords ||
        file_size != static_cast<std::streamoff>(expected_size)) {
        return std::nullopt;
    }

    std::vector<u32> spirv(header.word_count);
    file.read(reinterpret_cast<char*>(spirv.data()),
              static_cast<std::streamsize>(spirv.size() * sizeof(u32)));
    if (!file || spirv.front() != SpirvMagic || Digest(spirv) != header.digest) {
        return std::nullopt;
    }
    return spirv;
}

bool HostShaderCache::Store(std::string_view shader_name, std::string_view generation,
                            std::string_view permutation, std::span<const u32> spirv) {
    std::scoped_lock lock{mutex};
    if (!IsSafeName(permutation, false) || spirv.empty() || spirv.size() > MaxSpirvWords ||
        spirv.front() != SpirvMagic) {
        return false;
    }
    const auto generation_path = PrepareGenerationLocked(shader_name, generation);
    if (!generation_path) {
        return false;
    }

    const auto destination = *generation_path / (std::string{permutation} + ".spv");
    const CacheHeader header{
        .magic = CacheMagic,
        .version = CacheVersion,
        .word_count = static_cast<u32>(spirv.size()),
        .digest = Digest(spirv),
    };
    const auto temporary = TemporaryPath(destination);
    std::error_code ec;
    std::ofstream file{temporary, std::ios::binary | std::ios::trunc};
    if (!file) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(spirv.data()),
               static_cast<std::streamsize>(spirv.size_bytes()));
    file.flush();
    if (!file) {
        file.close();
        std::filesystem::remove(temporary, ec);
        return false;
    }
    file.close();

    std::filesystem::remove(destination, ec);
    if (ec) {
        std::filesystem::remove(temporary, ec);
        return false;
    }
    return Publish(temporary, destination);
}

} // namespace Storage
