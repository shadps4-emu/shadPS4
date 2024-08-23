#include "common/logging/log.h"
#include "memory_patcher.h"

namespace MemoryPatcher {

uintptr_t g_eboot_address;

std::vector<patchInfo> pending_patches;

void AddPatchToQueue(patchInfo patchToAdd) {
    pending_patches.push_back(patchToAdd);
}

void ApplyPendingPatches() {

    for (size_t i = 0; i < pending_patches.size(); ++i) {
        patchInfo currentPatch = pending_patches[i];
        PatchMemory(currentPatch.modNameStr, currentPatch.offsetStr, currentPatch.valueStr,
                    currentPatch.isOffset);
    }

    pending_patches.clear();
}

void PatchMemory(std::string modNameStr, std::string offsetStr, std::string valueStr,
                 bool isOffset) {
    // Send a request to modify the process memory.
    void* cheatAddress = nullptr;

    if (isOffset) {
        cheatAddress = reinterpret_cast<void*>(g_eboot_address + std::stoi(offsetStr, 0, 16));
    } else {
        cheatAddress =
            reinterpret_cast<void*>(g_eboot_address + (std::stoi(offsetStr, 0, 16) - 0x400000));
    }

    std::vector<unsigned char> bytePatch;

    for (size_t i = 0; i < valueStr.length(); i += 2) {
        unsigned char byte =
            static_cast<unsigned char>(std::strtol(valueStr.substr(i, 2).c_str(), nullptr, 16));

        bytePatch.push_back(byte);
    }
    std::memcpy(cheatAddress, bytePatch.data(), bytePatch.size());

    LOG_INFO(Loader, "Applied patch:{}, Offset:{}, Value:{}", modNameStr, (uintptr_t)cheatAddress,
             valueStr);
}

} // namespace MemoryPatcher