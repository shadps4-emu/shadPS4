#pragma once
#include "../Loader/Elf.h"

class ElfViewer {
private:
	Elf* elf;
public:
	ElfViewer(Elf* elf);
	void display(bool enabled);//display imgui window

};