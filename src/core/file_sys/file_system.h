// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "common/types.h"

enum FileAccess {
    FILEACCESS_NONE = 0,
    FILEACCESS_READ = 1,
    FILEACCESS_WRITE = 2,
};

enum FileMove { FILEMOVE_BEGIN = 0, FILEMOVE_CURRENT = 1, FILEMOVE_END = 2 };

class IFileSystem {
public:
    virtual ~IFileSystem() {}

    virtual int OpenFile(std::string filename, FileAccess access) = 0;
    virtual void CloseFile(u32 handle) = 0;
    virtual size_t ReadFile(u32 handle, u8* pointer, s64 size) = 0;
    virtual size_t WriteFile(u32 handle, const u8* pointer, s64 size) = 0;
    virtual size_t SeekFile(u32 handle, s32 position, FileMove type) = 0;
    virtual bool OwnsHandle(u32 handle) = 0;
    virtual bool MkDir(const std::string& dirname) = 0;
    virtual bool RmDir(const std::string& dirname) = 0;
    virtual int RenameFile(const std::string& from, const std::string& to) = 0;
    virtual bool RemoveFile(const std::string& filename) = 0;
};