// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// What the net kernel glue (src/core/libraries/net/net_kernel.cpp) needs from the kernel,
// without the kernel: its errno and its file table. The table follows fs.cpp (lowest free
// slot first, entries deleted on release); the rest of fs.cpp needs the whole file system.

#include <mutex>

#include "core/file_sys/fs.h"
#include "core/libraries/kernel/kernel.h"

namespace Libraries::Kernel {

s32* PS4_SYSV_ABI __Error() {
    thread_local s32 error = 0;
    return &error;
}

} // namespace Libraries::Kernel

namespace Core::FileSys {

int HandleTable::CreateHandle() {
    std::scoped_lock lock{m_mutex};
    auto* file = new File{};
    for (size_t index = 0; index < m_files.size(); ++index) {
        if (m_files[index] == nullptr) {
            m_files[index] = file;
            return static_cast<int>(index);
        }
    }
    m_files.push_back(file);
    return static_cast<int>(m_files.size() - 1);
}

void HandleTable::DeleteHandle(int d) {
    std::scoped_lock lock{m_mutex};
    delete m_files.at(d);
    m_files[d] = nullptr;
}

File* HandleTable::GetFile(int d) {
    std::scoped_lock lock{m_mutex};
    if (d < 0 || static_cast<size_t>(d) >= m_files.size()) {
        return nullptr;
    }
    return m_files[d];
}

void HandleTable::CreateStdHandles() {
    // Descriptors 0-2, so net objects start at 3 as in the emulator.
    for (int i = 0; i < 3; ++i) {
        const int fd = CreateHandle();
        auto* file = GetFile(fd);
        file->type = FileType::Device;
        file->is_opened = true;
    }
}

} // namespace Core::FileSys
