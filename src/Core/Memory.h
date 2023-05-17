#pragma once

namespace Memory
{
	static u64 get_aligned_size(const elf_program_header* phdr);
	static u64 calculate_base_size(const elf_header* ehdr, const elf_program_header* phdr);

	namespace VirtualMemory {
		u64 memory_alloc(u64 address, u64 size);
	}
}