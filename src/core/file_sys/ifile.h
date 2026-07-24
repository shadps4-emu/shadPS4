// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "common/io_file.h"
#include "common/types.h"

namespace Core::FileSys {

/// Policy declared by an IFile backend for host mmap.
enum class MmapPolicy {
    Native,      // Backed by a real host file,mmap the fd directly.
    Copy,        // Backend can populate anon pages on demand (decompression, etc.).
    Unsupported, // Caller must fall back to buffered reads.
};

struct DirEntry {
    std::string name;
    bool is_directory{false};
    u64 size{0};
};

class IFile {
public:
    virtual ~IFile() = default;

    virtual s64 Read(void* dst, u64 size) = 0;
    virtual s64 Write(const void* src, u64 size) = 0;
    virtual bool Seek(s64 offset, Common::FS::SeekOrigin origin) = 0;
    virtual u64 Tell() const = 0;
    virtual u64 Size() const = 0;
    virtual bool Flush() = 0;
    virtual bool IsOpen() const = 0;

    virtual std::optional<std::filesystem::path> GetHostPath() const {
        return std::nullopt;
    }

    virtual Common::FS::IOFile* GetHostFile() {
        return nullptr;
    }

    virtual MmapPolicy GetMmapPolicy() const {
        return MmapPolicy::Unsupported;
    }

    virtual bool IsReadOnly() const {
        return true;
    }
};

class IDirectory {
public:
    virtual ~IDirectory() = default;
    virtual bool Next(DirEntry& out) = 0;
    virtual void Rewind() = 0;
};

class IBackend {
public:
    virtual ~IBackend() = default;

    virtual bool Exists(std::string_view rel_path) = 0;
    virtual bool IsDirectory(std::string_view rel_path) = 0;

    virtual std::unique_ptr<IFile> Open(std::string_view rel_path, bool writable) = 0;
    virtual std::unique_ptr<IDirectory> OpenDir(std::string_view rel_path) = 0;

    virtual bool IsReadOnly() const = 0;

    virtual std::optional<std::filesystem::path> RootHostPath() const {
        return std::nullopt;
    }
};

} // namespace Core::FileSys
