#include "PKG.h"
#include "../../core/FsFile.h"
#include <direct.h> 
#include <QString>
#include <QDir>

PKG::PKG()
{
}


PKG::~PKG()
{
}

bool PKG::open(const std::string& filepath) {
	FsFile file;
	if (!file.Open(filepath, fsRead))
	{
		return false;
	}
	pkgSize = file.getFileSize();
	PKGHeader pkgheader;
	file.ReadBE(pkgheader);
	//we have already checked magic should be ok

	//find title id it is part of pkg_content_id starting at offset 0x40
	file.Seek(0x47, fsSeekSet);//skip first 7 characters of content_id 
	file.Read(&pkgTitleID, sizeof(pkgTitleID));

	file.Close();

	return true;
}
bool PKG::extract(const std::string& filepath, const std::string& extractPath, std::string& failreason)
{
		this->extractPath = extractPath;
		FsFile file;
		if (!file.Open(filepath, fsRead))
		{
			return false;
		}
		pkgSize = file.getFileSize();
		PKGHeader pkgheader;
		file.ReadBE(pkgheader);

		if (pkgheader.pkg_size > pkgSize)
		{
			failreason = "PKG file size is different";
			return false;
		}
		if ((pkgheader.pkg_content_size + pkgheader.pkg_content_offset) > pkgheader.pkg_size)
		{
			failreason = "Content size is bigger than pkg size";
			return false;
		}
		file.Seek(0, fsSeekSet);
		pkg = (U08*)mmap(pkgSize, file.fileDescr());
		if (pkg == nullptr)
		{
			failreason = "Can't allocate size for image";
			return false;
		}

		file.Read(pkg, pkgSize);
		
		U32 offset = pkgheader.pkg_table_entry_offset;
		U32 n_files = pkgheader.pkg_table_entry_count;


		for (int i = 0; i < n_files; i++) {
			PKGEntry entry = (PKGEntry&)pkg[offset + i * 0x20];
			ReadBE(entry);
			//try to figure out the name
			std::string name = getEntryNameByType(entry.id);
			if (!name.empty())
			{
				QString filepath= QString::fromStdString(extractPath+name);
				QDir dir = QFileInfo(filepath).dir();
				if (!dir.exists()) {
					dir.mkpath(dir.path());
				}
				FsFile out;
				out.Open(extractPath + name, fsWrite);
				out.Write(pkg + entry.offset, entry.size);
				out.Close();
			}
			else
			{
				//just print with id
				FsFile out;
				out.Open(extractPath + std::to_string(entry.id), fsWrite);
				out.Write(pkg + entry.offset, entry.size);
				out.Close();
			}
		}
		//extract pfs_image.dat
		FsFile out;
		out.Open(extractPath + "pfs_image.dat", fsWrite);
		out.Write(pkg + pkgheader.pfs_image_offset, pkgheader.pfs_image_size);
		out.Close();
		munmap(pkg);
		return true;
}