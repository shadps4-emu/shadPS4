#include "FsFile.h"

FsFile::FsFile()
{
	m_file = nullptr;
}

FsFile::FsFile(const std::string& path, fsOpenMode mode)
{
	Open(path, mode);
}

bool FsFile::Open(const std::string& path, fsOpenMode mode)
{
	Close();
	#ifdef _WIN64
		fopen_s(&m_file, path.c_str(), getOpenMode(mode));
	#else
		m_file = std::fopen(path.c_str(), getOpenMode(mode));
	#endif
	return IsOpen();
}

bool FsFile::Close()
{
	if (!IsOpen() || std::fclose(m_file) != 0) {
		m_file = nullptr;
		return false;
	}

	m_file = nullptr;
	return true;
}

FsFile::~FsFile()
{
	Close();
}

bool FsFile::Write(const void* src, u64 size)
{
	if (!IsOpen() || std::fwrite(src, 1, size, m_file) != size) {
		return false;
	}
	return true;
}

bool FsFile::Read(void* dst, u64 size)
{
	if (!IsOpen() || std::fread(dst, 1, size, m_file) != size) {
		return false;
	}
	return true;
}

u32 FsFile::ReadBytes(void* dst, u64 size)
{
	return std::fread(dst, 1, size, m_file);
}

bool FsFile::Seek(s64 offset, fsSeekMode mode)
{
	#ifdef _WIN64
		if (!IsOpen() || _fseeki64(m_file, offset, getSeekMode(mode)) != 0) {
			return false;
		}
	#else
		if (!IsOpen() || fseeko64(m_file, offset, getSeekMode(mode)) != 0) {
			return false;
		}
	#endif
	return true;
}

u64 FsFile::Tell() const
{
	if (IsOpen()) {
		#ifdef _WIN64
			return _ftelli64(m_file);
		#else
			return ftello64(m_file);
		#endif
	}
	else {
		return -1;
	}
}
u64 FsFile::getFileSize()
{
	#ifdef _WIN64
		u64 pos = _ftelli64(m_file);
		if (_fseeki64(m_file, 0, SEEK_END) != 0) {

			return 0;
		}

		u64 size = _ftelli64(m_file);
		if (_fseeki64(m_file, pos, SEEK_SET) != 0) {

			return 0;
		}
	#else
		u64 pos = ftello64(m_file);
		if (fseeko64(m_file, 0, SEEK_END) != 0) {

			return 0;
		}

		u64 size = ftello64(m_file);
		if (fseeko64(m_file, pos, SEEK_SET) != 0) {

			return 0;
		}
	#endif
	return size;
}

bool FsFile::IsOpen() const
{
	return m_file != nullptr;
}

