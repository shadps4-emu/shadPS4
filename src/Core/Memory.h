#pragma once

constexpr u64 SYSTEM_RESERVED = 0x800000000u;
constexpr u64 CODE_BASE_OFFSET = 0x100000000u;

namespace Memory
{
	namespace VirtualMemory {
		u64 memory_alloc(u64 address, u64 size);
	}
}