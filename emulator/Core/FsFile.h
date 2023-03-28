#pragma once
#include <cstdio>
#include <string>
#include "../Types.h"

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
	bool Read(void* dst, u64 size);
	u32 ReadBytes(void* dst, u64 size);
	bool Write(const void* src, u64 size);
	bool Seek(s64 offset, fsSeekMode mode);
	u64 getFileSize();
	u64 Tell() const;
	~FsFile();

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

