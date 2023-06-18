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

struct ModuleInfo
{
	std::string name;
	union
	{
		u64 value;
		struct
		{
			u32 name_offset;
			u08 version_minor;
			u08 version_major;
			u16 id;
		};
	};
	std::string enc_id;
};

struct LibraryInfo
{
	std::string name;
	union
	{
		u64 value;
		struct
		{
			u32 name_offset;
			u16 version;
			u16 id;
		};
	};
	std::string enc_id;
};

struct DynamicModuleInfo
{
	void* hash_table = nullptr;
	u64 hash_table_size = 0;

	char* str_table = nullptr;
	u64 str_table_size = 0;

	elf_symbol* symbol_table = nullptr;
	u64 symbol_table_total_size = 0;
	u64 symbol_table_entries_size = 0;

	u64 init_virtual_addr = 0;
	u64 fini_virtual_addr = 0;
	u64 pltgot_virtual_addr = 0;
	u64 init_array_virtual_addr = 0;
	u64 fini_array_virtual_addr = 0;
	u64 preinit_array_virtual_addr = 0;
	u64 init_array_size = 0;
	u64 fini_array_size = 0;
	u64 preinit_array_size = 0;

	elf_relocation* jmp_relocation_table = nullptr;
	u64 jmp_relocation_table_size = 0;
	s64 jmp_relocation_type = 0;

	elf_relocation* relocation_table = nullptr;
	u64 relocation_table_size = 0;
	u64 relocation_table_entries_size = 0;

	u64 debug = 0;
	u64 textrel = 0;
	u64 flags = 0;

	std::vector<const char*> needed;
	std::vector<ModuleInfo>    import_modules;
	std::vector<ModuleInfo>    export_modules;
	std::vector<LibraryInfo>   import_libs;
	std::vector<LibraryInfo>   export_libs;

	std::string filename;//filename with absolute path

};

class Linker
{
public:
	Linker();
	virtual ~Linker();

	Module* LoadModule(const std::string& elf_name);
	Module* FindModule(/*u32 id*/);
	void LoadModuleToMemory(Module* m);
	void LoadDynamicInfo(Module* m);
	void LoadSymbols(Module* m);

private:
	const ModuleInfo* FindModule(const Module& m, const std::string& id);
	const LibraryInfo* FindLibrary(const Module& program, const std::string& id);
	std::vector<Module*> m_modules;
};