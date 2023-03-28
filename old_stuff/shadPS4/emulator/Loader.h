#pragma once

#include <string>

enum FileTypes
{
	FILETYPE_UNKNOWN,
	FILETYPE_PKG
};
FileTypes detectFileType(const std::string& filepath);
