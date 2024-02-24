// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>

class IOFile {
    FILE* handle = nullptr;
    static inline std::filesystem::path appData =
        ""; // Directory for holding app data. AppData on Windows

public:
    IOFile() : handle(nullptr) {}
    IOFile(FILE* handle) : handle(handle) {}
    IOFile(const std::filesystem::path& path, const char* permissions = "rb");

    bool isOpen() {
        return handle != nullptr;
    }
    bool open(const std::filesystem::path& path, const char* permissions = "rb");
    bool open(const char* filename, const char* permissions = "rb");
    void close();

    std::pair<bool, std::size_t> read(void* data, std::size_t length, std::size_t dataSize);
    std::pair<bool, std::size_t> readBytes(void* data, std::size_t count);

    std::pair<bool, std::size_t> write(const void* data, std::size_t length, std::size_t dataSize);
    std::pair<bool, std::size_t> writeBytes(const void* data, std::size_t count);

    std::optional<std::uint64_t> size();

    bool seek(std::int64_t offset, int origin = SEEK_SET);
    bool rewind();
    bool flush();
    FILE* getHandle();
    static void setAppDataDir(const std::filesystem::path& dir);
    static std::filesystem::path getAppData() {
        return appData;
    }

    // Sets the size of the file to "size" and returns whether it succeeded or not
    bool setSize(std::uint64_t size);
};
