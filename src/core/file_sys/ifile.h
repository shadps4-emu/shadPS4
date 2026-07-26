// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "common/concepts.h"
#include "common/io_file.h"
#include "common/types.h"

namespace Core::FileSys {

// Policy declared by an IFile backend for host mmap.
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

    virtual std::optional<std::filesystem::path> GetMetadataHostPath() const {
        return GetHostPath();
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

    virtual bool IsWriteOnly() const {
        return false;
    }
};

class FileReader {
public:
    FileReader() = default;
    explicit FileReader(std::unique_ptr<IFile> handle) : m_handle(std::move(handle)) {}

    void Reset(std::unique_ptr<IFile> handle) {
        m_handle = std::move(handle);
    }

    bool IsOpen() const {
        return m_handle && m_handle->IsOpen();
    }

    IFile* Get() const {
        return m_handle.get();
    }

    template <typename T>
    bool ReadObject(T& obj) {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        static_assert(!std::is_pointer_v<T>, "T must not be a pointer");
        if (!IsOpen()) {
            return false;
        }
        return m_handle->Read(&obj, sizeof(T)) == static_cast<s64>(sizeof(T));
    }

    template <typename T>
    size_t ReadRaw(void* dst, size_t count) {
        if (!IsOpen() || count == 0) {
            return 0;
        }
        const auto bytes_requested = count * sizeof(T);
        const s64 got = m_handle->Read(dst, bytes_requested);
        if (got <= 0) {
            return 0;
        }
        return static_cast<size_t>(got) / sizeof(T);
    }

    template <typename T>
    size_t Read(T& data) {
        if constexpr (Common::IsContiguousContainer<T>) {
            using ContiguousType = typename T::value_type;
            static_assert(std::is_trivially_copyable_v<ContiguousType>,
                          "container element type must be trivially copyable");
            return ReadRaw<ContiguousType>(data.data(), data.size());
        } else {
            return ReadObject(data) ? 1 : 0;
        }
    }

    bool Seek(s64 offset, Common::FS::SeekOrigin origin = Common::FS::SeekOrigin::SetOrigin) {
        if (!IsOpen()) {
            return false;
        }
        return m_handle->Seek(offset, origin);
    }

    s64 Tell() const {
        if (!IsOpen()) {
            return -1;
        }
        return static_cast<s64>(m_handle->Tell());
    }

    u64 GetSize() const {
        if (!IsOpen()) {
            return 0;
        }
        return m_handle->Size();
    }

private:
    std::unique_ptr<IFile> m_handle;
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
    virtual std::unique_ptr<IFile> Open(std::string_view rel_path,
                                        Common::FS::FileAccessMode mode) = 0;
    virtual std::unique_ptr<IDirectory> OpenDir(std::string_view rel_path) = 0;

    virtual bool IsReadOnly() const = 0;
    virtual std::optional<std::filesystem::path> RootHostPath() const {
        return std::nullopt;
    }
};

} // namespace Core::FileSys
