// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/io_file.h"
#include "common/types.h"
#include "loader.h"

namespace Loader {

FileTypes DetectFileType(const std::filesystem::path& filepath) {
    // No file loaded
    if (filepath.empty()) {
        return FileTypes::Unknown;
    }
    Common::FS::IOFile file;
    file.Open(filepath, Common::FS::FileAccessMode::Read);
    file.Seek(0);
    u32 magic;
    file.Read(magic);
    file.Close();
    switch (magic) {
    case PkgMagic:
        return FileTypes::Pkg;
    }
    return FileTypes::Unknown;
}

} // namespace Loader
