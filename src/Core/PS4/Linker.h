#pragma once
#include "../../Loader/Elf.h"
#include <vector>

struct DynamicModuleInfo;

/*this struct keeps neccesary info about loaded modules.Main executeable is included too as well*/
struct Module
{
	Elf* elf = nullptr;
	u64 aligned_base_size = 0;
	u64 base_virtual_addr = 0; //base virtual address

	void* m_dynamic = nullptr;
	void* m_dynamic_data = nullptr;
	DynamicModuleInfo* dynamic_info = nullptr;
};

struct DynamicModuleInfo
{
	void* hash_table = nullptr;
	u64 hash_table_size = 0;

	char* str_table = nullptr;
	u64 str_table_size = 0;

	elf_symbol* symbol_table = nullptr;
	u64 symbol_table_total_size = 0;

	u64 init_virtual_addr = 0;
	u64 fini_virtual_addr = 0;
	u64 pltgot_virtual_addr = 0;

	elf_relocation* jmp_relocation_table = nullptr;
	u64 jmp_relocation_table_size = 0;
	s64 jmp_relocation_type = 0;

	elf_relocation* relocation_table = nullptr;
	u64 relocation_table_size = 0;
	u64 relocation_table_entries_size = 0;
};

class Linker
{
public:
	Linker();
	virtual ~Linker();

	Module* LoadModule(const std::string& elf_name);
	Module* FindModule(/*u32 id*/);
	void LoadModuleToMemory(Module* m);
	void LoadDynamicInfo(Module* program);

private:
	std::vector<Module*> m_modules;
};