#pragma once
#include "../types.h"

constexpr u64 SYSTEM_RESERVED = 0x800000000u;
constexpr u64 CODE_BASE_OFFSET = 0x100000000u;

namespace Memory
{
	enum class MemoryMode : u32
	{
		NoAccess = 0,
		Read = 1,
		Write = 2,
		ReadWrite =3,
		Execute = 4,
		ExecuteRead = 5,
		ExecuteWrite = 6,
		ExecuteReadWrite = 7,
	};

	namespace VirtualMemory {
		u64 memory_alloc(u64 address, u64 size, MemoryMode mode);
	}
}