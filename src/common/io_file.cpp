#include "io_file.h"

// #include "helpers.hpp"

#ifdef _MSC_VER
// 64 bit offsets for MSVC
#define fseeko _fseeki64
#define ftello _ftelli64
#define fileno _fileno

#pragma warning(disable : 4996)
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef WIN32
#include <io.h> // For _chsize_s
#else
#include <unistd.h> // For ftruncate
#endif

IOFile::IOFile(const std::filesystem::path& path, const char* permissions) : handle(nullptr) {
    open(path, permissions);
}

bool IOFile::open(const std::filesystem::path& path, const char* permissions) {
    const auto str =
        path.string(); // For some reason converting paths directly with c_str() doesn't work
    return open(str.c_str(), permissions);
}

bool IOFile::open(const char* filename, const char* permissions) {
    // If this IOFile is already bound to an open file descriptor, release the file descriptor
    // To avoid leaking it and/or erroneously locking the file
    if (isOpen()) {
        close();
    }

    handle = std::fopen(filename, permissions);
    return isOpen();
}

void IOFile::close() {
    if (isOpen()) {
        fclose(handle);
        handle = nullptr;
    }
}

std::pair<bool, std::size_t> IOFile::read(void* data, std::size_t length, std::size_t dataSize) {
    if (!isOpen()) {
        return {false, std::numeric_limits<std::size_t>::max()};
    }

    if (length == 0)
        return {true, 0};
    return {true, std::fread(data, dataSize, length, handle)};
}

std::pair<bool, std::size_t> IOFile::write(const void* data, std::size_t length,
                                           std::size_t dataSize) {
    if (!isOpen()) {
        return {false, std::numeric_limits<std::size_t>::max()};
    }

    if (length == 0) {
        return {true, 0};
    } else {
        return {true, std::fwrite(data, dataSize, length, handle)};
    }
}

std::pair<bool, std::size_t> IOFile::readBytes(void* data, std::size_t count) {
    return read(data, count, sizeof(std::uint8_t));
}
std::pair<bool, std::size_t> IOFile::writeBytes(const void* data, std::size_t count) {
    return write(data, count, sizeof(std::uint8_t));
}

std::optional<std::uint64_t> IOFile::size() {
    if (!isOpen())
        return {};

    std::uint64_t pos = ftello(handle);
    if (fseeko(handle, 0, SEEK_END) != 0) {
        return {};
    }

    std::uint64_t size = ftello(handle);
    if ((size != pos) && (fseeko(handle, pos, SEEK_SET) != 0)) {
        return {};
    }

    return size;
}

bool IOFile::seek(std::int64_t offset, int origin) {
    if (!isOpen() || fseeko(handle, offset, origin) != 0)
        return false;

    return true;
}

bool IOFile::flush() {
    if (!isOpen() || fflush(handle))
        return false;

    return true;
}

bool IOFile::rewind() {
    return seek(0, SEEK_SET);
}
FILE* IOFile::getHandle() {
    return handle;
}

void IOFile::setAppDataDir(const std::filesystem::path& dir) {
    // if (dir == "")
    //     Helpers::panic("Failed to set app data directory");
    appData = dir;
}

bool IOFile::setSize(std::uint64_t size) {
    if (!isOpen())
        return false;
    bool success;

#ifdef WIN32
    success = _chsize_s(_fileno(handle), size) == 0;
#else
    success = ftruncate(fileno(handle), size) == 0;
#endif
    fflush(handle);
    return success;
}