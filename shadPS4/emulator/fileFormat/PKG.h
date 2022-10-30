#pragma once
#include <string>
#include "../../Types.h"

struct PKGHeader {
	/*BE*/U32 magic;                  // Magic
};

class PKG
{
private:
	U08* pkg;
	U64 pkgSize = 0;
	S08 pkgTitleID[9];

public:
	PKG();
	~PKG();
	bool open(const std::string& filepath);
	U64 getPkgSize()
	{
		return pkgSize;
	}
	std::string getTitleID()
	{
		return std::string(pkgTitleID);
	}
};

