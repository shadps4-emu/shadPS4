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