#include "../Loader/Elf.h"
#include <windows.h>
#include "../Util/Log.h"

namespace Memory
{

	static u64 get_aligned_size(const elf_program_header* phdr)
	{
		return (phdr->p_align != 0 ? (phdr->p_memsz + (phdr->p_align - 1)) & ~(phdr->p_align - 1) : phdr->p_memsz);
	}

	static u64 calculate_base_size(const elf_header* ehdr, const elf_program_header* phdr)
	{
		u64 base_size = 0;
		for (u16 i = 0; i < ehdr->e_phnum; i++)
		{
			if (phdr[i].p_memsz != 0 && (phdr[i].p_type == PT_LOAD || phdr[i].p_type == PT_SCE_RELRO))
			{
				auto phdrh = phdr + i;
				u64 last_addr = phdr[i].p_vaddr + get_aligned_size(phdrh);
				if (last_addr > base_size)
				{
					base_size = last_addr;
				}
			}
		}
		return base_size;
	}

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