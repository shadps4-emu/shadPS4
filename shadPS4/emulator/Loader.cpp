#include "Loader.h"
#include "../core/FsFile.h"

FileTypes detectFileType(const std::string& filepath)
{
	if (filepath.size() == 0)//no file loaded
	{
		return FILETYPE_UNKNOWN;
	}
	FsFile file;
	file.Open(filepath, fsRead);
	file.Seek(0, fsSeekSet);
	U32 magic;
	file.Read(&magic, sizeof(magic));
	file.Close();
	ReadBE(magic);//magic is BE make it LE
	switch (magic)
	{
	case 0x7F434E54://PS4 PKG
		return FILETYPE_PKG;
	}
	return FILETYPE_UNKNOWN;

}
