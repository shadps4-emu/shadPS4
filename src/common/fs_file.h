#pragma once

#include <bit>
#include <cstdio>
#include <span>
#include <string>
#include <vector>

#include "common/types.h"

namespace Common::FS {

enum class OpenMode : u32 { Read = 0x1, Write = 0x2, ReadWrite = Read | Write };

enum class SeekMode : u32 {
    Set,
    Cur,
    End,
};

class File {
public:
    File();
    explicit File(const std::string& path, OpenMode mode = OpenMode::Read);
    ~File();

    bool open(const std::string& path, OpenMode mode = OpenMode::Read);
    bool close();
    bool read(void* data, u64 size) const;
    bool write(std::span<const u08> data);
    bool seek(s64 offset, SeekMode mode);
    u64 getFileSize();
    u64 tell() const;

    template <typename T>
    bool read(T& data) const {
        return read(&data, sizeof(T));
    }

    template <typename T>
    bool read(std::vector<T>& data) const {
        return read(data.data(), data.size() * sizeof(T));
    }

    bool isOpen() const {
        return m_file != nullptr;
    }

    const char* getOpenMode(OpenMode mode) const {
        switch (mode) {
        case OpenMode::Read:
            return "rb";
        case OpenMode::Write:
            return "wb";
        case OpenMode::ReadWrite:
            return "r+b";
        default:
            return "r";
        }
    }

    int getSeekMode(SeekMode mode) const {
        switch (mode) {
        case SeekMode::Set:
            return SEEK_SET;
        case SeekMode::Cur:
            return SEEK_CUR;
        case SeekMode::End:
            return SEEK_END;
        default:
            return SEEK_SET;
        }
    }

    [[nodiscard]] std::FILE* fileDescr() const {
        return m_file;
    }

private:
    std::FILE* m_file{};
};

} // namespace Common::FS
