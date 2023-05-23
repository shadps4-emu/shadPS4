#pragma once
#include "../../Loader/Elf.h"

/*this struct keeps neccesary info about loaded modules.Main executeable is included too as well*/
struct Module
{
	Elf* elf = nullptr;
};

class Linker
{
public:
	Linker();
	virtual ~Linker();

	Module* LoadModule(const std::string& elf_name);
};