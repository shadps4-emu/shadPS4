#include "memory_patcher.h"
#include "common/logging/log.h"

namespace MemoryPatcher 
{

uintptr_t g_eboot_address;

std::vector<patchInfo> pending_patches;

void AddPatchToQueue(patchInfo patchToAdd) {
    pending_patches.push_back(patchToAdd);
}

void ApplyPendingPatches() {

	for (size_t i = 0; i < pending_patches.size(); ++i) {
        patchInfo currentPatch = pending_patches[i];
        LOG_INFO(Loader, "loading patch {}", i);
        PatchMemory(currentPatch.modNameStr, currentPatch.offsetStr, currentPatch.valueStr);
    }

    pending_patches.clear();
}

void PatchMemory(std::string modNameStr, std::string offsetStr, std::string valueStr) {
    // Send a request to modify the process memory.
    void* cheatAddress =
        reinterpret_cast<void*>(g_eboot_address + std::stoi(offsetStr, 0, 16));

    std::vector<unsigned char> bytePatch;

    for (size_t i = 0; i < valueStr.length(); i += 2) {
        unsigned char byte =
            static_cast<unsigned char>(std::strtol(valueStr.substr(i, 2).c_str(), nullptr, 16));

        bytePatch.push_back(byte);
    }
    std::memcpy(cheatAddress, bytePatch.data(), bytePatch.size());

    LOG_INFO(Loader, "Applied patch:{}, Offset:{}, Value:{}", modNameStr, (uintptr_t)cheatAddress, valueStr);
}

}