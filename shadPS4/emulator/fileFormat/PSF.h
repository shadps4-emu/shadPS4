#pragma once

#include <unordered_map>
#include <vector>
#include "../../types.h"

struct PSFHeader {
	U32 magic;  //big endian
	U32 version;
	U32 keyTableOffset;
	U32 dataTableOffset;
	U32 indexTableEntries;
};

struct PSFEntry {
	enum Fmt : U16 {
		TEXT_RAW = 0x0400, // String in UTF-8 format and not NULL terminated
		TEXT_NORMAL = 0x0402, // String in UTF-8 format and NULL terminated
		INTEGER = 0x0404, // Unsigned 32-bit integer
	};

	U16 keyOffset;
	U16 param_fmt;//big endian
	U32 paramLen;
	U32 paramMaxLen;
	U32 dataOffset;
};

class PSF
{
	std::vector<U08> psf;
	std::unordered_map<std::string, std::string> map_strings;
	std::unordered_map<std::string, U32> map_integers;

public:
	PSF();
	~PSF();
	bool open(const std::string& filepath);

	// Access data
	std::string get_string(const std::string& key);
	U32 get_integer(const std::string& key);
};

