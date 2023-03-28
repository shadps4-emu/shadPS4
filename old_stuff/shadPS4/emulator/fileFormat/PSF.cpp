#include "PSF.h"
#include "../../Core/FsFile.h"
#include <iostream>
#include <fstream>

PSF::PSF()
{
}


PSF::~PSF()
{
}
bool PSF::open(const std::string& filepath) {
	FsFile file;
	if (!file.Open(filepath, fsRead))
	{
		return false;
	}

	const U64 psfSize = file.getFileSize();
	psf.resize(psfSize);
	file.Seek(0, fsSeekSet);
	file.Read(&psf[0], psfSize);

	// Parse file contents
	const auto& header = (PSFHeader&)psf[0];
	for (U32 i = 0; i < header.indexTableEntries; i++) {
		const U32 offset = sizeof(PSFHeader) + i * sizeof(PSFEntry);
		auto& entry = (PSFEntry&)psf[offset];

		std::string key = (char*)&psf[header.keyTableOffset + entry.keyOffset];
		ReadBE(entry.param_fmt);//param_fmt is big endian convert it (this convert the original entry maybe we should store it elsewhere?)
		if (entry.param_fmt == PSFEntry::Fmt::TEXT_RAW ||
			entry.param_fmt == PSFEntry::Fmt::TEXT_NORMAL) {
			map_strings[key] = (char*)&psf[header.dataTableOffset + entry.dataOffset];
		}
		if (entry.param_fmt == PSFEntry::Fmt::INTEGER) {
			map_integers[key] = (U32&)psf[header.dataTableOffset + entry.dataOffset];
		}
	}
	//debug code print all keys
	std::ofstream out;
	out.open("psf.txt", std::fstream::out | std::fstream::app);
	out << "---------------------------------------------" << "\n";
	for (auto stringkey : map_strings)
	{
		out << " " << stringkey.first << " : " << stringkey.second << "\n";
	}
	for (auto integerkey : map_integers)
	{
		out << " " << integerkey.first << " : " << integerkey.second << "\n";
	}
	out << "---------------------------------------------" << "\n";

	return true;
}
std::string PSF::get_string(const std::string& key) {
	if (map_strings.find(key) != map_strings.end()) {
		return map_strings.at(key);
	}
	return "";
}
U32 PSF::get_integer(const std::string& key)
{
	if (map_integers.find(key) != map_integers.end()) {
		return map_integers.at(key); //TODO std::invalid_argument exception if it fails?
	}
	return 0;
}