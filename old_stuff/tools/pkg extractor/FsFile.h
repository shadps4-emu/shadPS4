#pragma once
#include <cstdio>
#include <string>
#include "Types.h"

enum fsOpenMode
{
	fsRead = 0x1,
	fsWrite = 0x2,
	fsReadWrite = fsRead | fsWrite
};

enum fsSeekMode
{
	fsSeekSet,
	fsSeekCur,
	fsSeekEnd,
};

class FsFile
{
	std::FILE* m_file;
public:
	FsFile();
	FsFile(const std::string& path, fsOpenMode mode = fsRead);
	bool Open(const std::string& path, fsOpenMode mode = fsRead);
	bool IsOpen() const;
	bool Close();
	bool Read(void* dst, U64 size);
	U32 ReadBytes(void* dst, U64 size);
	bool Write(const void* src, U64 size);
	bool Seek(S64 offset, fsSeekMode mode);
	U64 getFileSize();
	U64 Tell() const;
	~FsFile();

	template< typename T > bool ReadBE(T& dst)
	{
		if (!Read(&dst, sizeof(T))) {
			return false;
		}
		::ReadBE(dst);
		return true;
	}

	const char* getOpenMode(fsOpenMode mode)
	{
		switch (mode) {
		case fsRead:        return "rb";
		case fsWrite:       return "wb";
		case fsReadWrite:   return "r+b";
		}

		return "r";
	}

	const int getSeekMode(fsSeekMode mode)
	{
		switch (mode) {
		case fsSeekSet:  return SEEK_SET;
		case fsSeekCur:  return SEEK_CUR;
		case fsSeekEnd:  return SEEK_END;
		}

		return SEEK_SET;
	}
	std::FILE* fileDescr()
	{
		return m_file;
	}
};

