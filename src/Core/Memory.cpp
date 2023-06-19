#include "../Core/PS4/Loader/Elf.h"
#include <windows.h>
#include "../Util/Log.h"

namespace Memory
{
	namespace VirtualMemory {

		u64 memory_alloc(u64 address, u64 size)
		{
			//TODO it supports only execute_read_write mode
			auto ptr = reinterpret_cast<uintptr_t>(VirtualAlloc(reinterpret_cast<LPVOID>(static_cast<uintptr_t>(address)), 
															    size,
																static_cast<DWORD>(MEM_COMMIT) | static_cast<DWORD>(MEM_RESERVE),
																PAGE_EXECUTE_READWRITE));

			if (ptr == 0)
			{
				auto err = static_cast<u32>(GetLastError());
				LOG_ERROR_IF(true,"VirtualAlloc() failed: 0x{:X}\n", err);
			}
			return ptr;
		}
	}

}