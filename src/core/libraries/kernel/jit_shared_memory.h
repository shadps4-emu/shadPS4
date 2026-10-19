// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <span>
#include <vector>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

void RegisterJitSharedMemory(Core::Loader::SymbolsResolver* sym);

#ifdef __linux__
// The emulator and its local processes assign JIT addresses from one shared page.
// Returns the page's host descriptor, creating the page on first use, or -1.
int GetJitFamilyDescriptor();
// Takes over the page inherited from the parent emulator. False if it is not a valid page.
bool JoinJitFamily(int host_fd);

// Host descriptors carrying guest JIT descriptors over a local socket: an object and its
// metadata for every guest descriptor. Keeps the objects alive and closes the metadata.
class JitDescriptorBatch {
public:
    JitDescriptorBatch() = default;
    ~JitDescriptorBatch();
    JitDescriptorBatch(const JitDescriptorBatch&) = delete;
    JitDescriptorBatch& operator=(const JitDescriptorBatch&) = delete;

    void Add(int object_fd, int metadata_fd, std::shared_ptr<const void> keep_alive);
    std::span<const int> HostDescriptors() const {
        return fds;
    }

private:
    std::vector<int> fds;
    std::vector<int> metadata_fds;
    std::vector<std::shared_ptr<const void>> objects;
};

// Returns 0 or a FreeBSD errno.
int ExportJitDescriptors(std::span<const s32> guest_fds, JitDescriptorBatch* batch);

struct JitImportResult {
    std::vector<s32> guest_fds;
    bool dropped = false; // part of the descriptors could not be imported
};
// Takes ownership of descriptors received in one message. Imports whole object/metadata pairs
// and closes the rest; any invalid pair rejects them all.
JitImportResult ImportJitDescriptors(std::span<const int> host_fds);
#endif

} // namespace Libraries::Kernel
