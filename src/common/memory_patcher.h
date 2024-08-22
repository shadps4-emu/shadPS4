#pragma once
#include <cstring>
#include <vector>
#include <string>

namespace MemoryPatcher 
{
	extern uintptr_t g_eboot_address;

	struct patchInfo {
        std::string modNameStr;
        std::string offsetStr;
        std::string valueStr;
	};

	extern std::vector<patchInfo> pending_patches;

	void AddPatchToQueue(patchInfo patchToAdd);
	void ApplyPendingPatches();

	void PatchMemory(std::string modNameStr, std::string offsetStr, std::string valueStr);


}