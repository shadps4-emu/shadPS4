#pragma once
#include <string>
#include "../../Types.h"
#include <io.h>
#include <windows.h>
#include <stdio.h>

struct PKGHeader {
	/*BE*/U32 magic;// Magic
	/*BE*/U32 pkg_type;
	/*BE*/U32 pkg_0x8; //unknown field
	/*BE*/U32 pkg_file_count;
	/*BE*/U32 pkg_table_entry_count;
	/*BE*/U16 pkg_sc_entry_count;
	/*BE*/U16 pkg_table_entry_count_2;// same as pkg_entry_count
	/*BE*/U32 pkg_table_entry_offset;//file table offset
	/*BE*/U32 pkg_sc_entry_data_size;
	/*BE*/U64 pkg_body_offset;//offset of PKG entries
	/*BE*/U64 pkg_body_size;//length of all PKG entries
	/*BE*/U64 pkg_content_offset;
	/*BE*/U64 pkg_content_size;
	U08 pkg_content_id[0x24];//packages' content ID as a 36-byte string
	U08 pkg_padding[0xC];//padding
	/*BE*/U32 pkg_drm_type;//DRM type
	/*BE*/U32 pkg_content_type;//Content type
	/*BE*/U32 pkg_content_flags;//Content flags
	/*BE*/U32 pkg_promote_size;
	/*BE*/U32 pkg_version_date;
	/*BE*/U32 pkg_version_hash;
	/*BE*/U32 pkg_0x088;
	/*BE*/U32 pkg_0x08C;
	/*BE*/U32 pkg_0x090;
	/*BE*/U32 pkg_0x094;
	/*BE*/U32 pkg_iro_tag;
	/*BE*/U32 pkg_drm_type_version;

	U08 pkg_zeroes_1[0x60];

	/* Digest table */
	U08 digest_entries1[0x20];     // sha256 digest for main entry 1
	U08 digest_entries2[0x20];     // sha256 digest for main entry 2
	U08 digest_table_digest[0x20]; // sha256 digest for digest table
	U08 digest_body_digest[0x20];  // sha256 digest for main table

	U08 pkg_zeroes_2[0x280];

	U32 pkg_0x400;

	U32 pfs_image_count;                // count of PFS images
	U64 pfs_image_flags;                // PFS flags
	U64 pfs_image_offset;               // offset to start of external PFS image
	U64 pfs_image_size;                 // size of external PFS image
	U64 mount_image_offset;
	U64 mount_image_size;
	U64 pkg_size;
	U32 pfs_signed_size;
	U32 pfs_cache_size;
	U08 pfs_image_digest[0x20];
	U08 pfs_signed_digest[0x20];
	U64 pfs_split_size_nth_0;
	U64 pfs_split_size_nth_1;

	U08 pkg_zeroes_3[0xB50];

	U08 pkg_digest[0x20];

};

inline void ReadBE(PKGHeader& s)
{
	ReadBE(s.magic);
	ReadBE(s.pkg_table_entry_offset);
	ReadBE(s.pkg_table_entry_count);
	ReadBE(s.pkg_content_offset);
	ReadBE(s.pkg_content_size);
	ReadBE(s.pfs_image_offset);
	ReadBE(s.pfs_image_size);
	ReadBE(s.pkg_size);
}

struct PKGEntry {
	U32 id;               // File ID, useful for files without a filename entry
	U32 filename_offset;  // Offset into the filenames table (ID 0x200) where this file's name is located
	U32 flags1;           // Flags including encrypted flag, etc
	U32 flags2;           // Flags including encryption key index, etc
	U32 offset;           // Offset into PKG to find the file
	U32 size;             // Size of the file
	U64 padding;          // blank padding
};

inline void ReadBE(PKGEntry& s)
{
	ReadBE(s.id);
	ReadBE(s.filename_offset);
	ReadBE(s.flags1);
	ReadBE(s.flags2);
	ReadBE(s.offset);
	ReadBE(s.size);
	ReadBE(s.padding);
}

class PKG
{
private:
	U08* pkg;
	U64 pkgSize = 0;
	S08 pkgTitleID[9];
	std::string extractPath;

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
		return std::string(pkgTitleID,9);
	}
	bool extract(const std::string& filepath, const std::string& extractPath, std::string& failreason);

	void* mmap(size_t sLength, std::FILE* nFd) {
		HANDLE hHandle;
		void* pStart;
		hHandle = CreateFileMapping(
			(HANDLE)_get_osfhandle(_fileno((nFd))),
			NULL,                    // default security
			PAGE_WRITECOPY,          // read/write access
			0,                       // maximum object size (high-order DWORD)
			0,                // maximum object size (low-order DWORD)
			NULL);                 // name of mapping object

		if (hHandle != NULL) {
			pStart = MapViewOfFile(hHandle, FILE_MAP_COPY, 0, 0, sLength);
		}
		if(pStart == NULL)
		{
			return nullptr;
		}
		return pStart;
	}
	int munmap(void* pStart) {
		if (UnmapViewOfFile(pStart) != 0)
			return FALSE;

		return TRUE;
	}
	typedef struct {
		U32 type;
		std::string name;
	} pkg_entry_value;

	std::string getEntryNameByType(U32 type)
	{
		pkg_entry_value entries[] = {
		{ 0x0001,						   "digests"                   },
		{ 0x0010,						   "entry_keys"                },
		{ 0x0020,						   "image_key"                 },
		{ 0x0080,						   "general_digests"           },
		{ 0x0100,						   "metas"                     },
		{ 0x0200,						   "entry_names"               },
		{ 0x0400,                          "license.dat"               },
		{ 0x0401,                          "license.info"              },
		{ 0x0402,                          "nptitle.dat"               },
		{ 0x0403,                          "npbind.dat"                },
		{ 0x0404,                          "selfinfo.dat"              },
	    { 0x0406,                          "imageinfo.dat"             },
		{ 0x0407,                          "target-deltainfo.dat"      },
		{ 0x0408,                          "origin-deltainfo.dat"      },
		{ 0x0409,                          "psreserved.dat"            },
		{ 0x1000,                          "param.sfo"                 },
		{ 0x1001,                          "playgo-chunk.dat"          },
		{ 0x1002,                          "playgo-chunk.sha"          },
		{ 0x1003,                          "playgo-manifest.xml"       },
		{ 0x1004,                          "pronunciation.xml"         },
		{ 0x1005,                          "pronunciation.sig"         },
		{ 0x1006,                          "pic1.png"                  },
		{ 0x1007,                          "pubtoolinfo.dat"           },
		{ 0x1008,                          "app/playgo-chunk.dat"      },
		{ 0x1009,                          "app/playgo-chunk.sha"      },
		{ 0x100A,                          "app/playgo-manifest.xml"   },
		{ 0x100B,                          "shareparam.json"           },
		{ 0x100C,                          "shareoverlayimage.png"     },
		{ 0x100D,                          "save_data.png"             },
		{ 0x100E,                          "shareprivacyguardimage.png"},
		{ 0x1200,                          "icon0.png"                 },
		{ 0x1201,                          "icon0_00.png"              },
		{ 0x1202,                          "icon0_01.png"              },
		{ 0x1203,                          "icon0_02.png"              },
		{ 0x1204,                          "icon0_03.png"              },
		{ 0x1205,                          "icon0_04.png"              },
		{ 0x1206,                          "icon0_05.png"              },
		{ 0x1207,                          "icon0_06.png"              },
		{ 0x1208,                          "icon0_07.png"              },
		{ 0x1209,                          "icon0_08.png"              },
		{ 0x120A,                          "icon0_09.png"              },
		{ 0x120B,                          "icon0_10.png"              },
		{ 0x120C,                          "icon0_11.png"              },
		{ 0x120D,                          "icon0_12.png"              },
		{ 0x120E,                          "icon0_13.png"              },
		{ 0x120F,                          "icon0_14.png"              },
		{ 0x1210,                          "icon0_15.png"              },
		{ 0x1211,                          "icon0_16.png"              },
		{ 0x1212,                          "icon0_17.png"              },
		{ 0x1213,                          "icon0_18.png"              },
		{ 0x1214,                          "icon0_19.png"              },
		{ 0x1215,                          "icon0_20.png"              },
		{ 0x1216,                          "icon0_21.png"              },
		{ 0x1217,                          "icon0_22.png"              },
		{ 0x1218,                          "icon0_23.png"              },
		{ 0x1219,                          "icon0_24.png"              },
		{ 0x121A,                          "icon0_25.png"              },
		{ 0x121B,                          "icon0_26.png"              },
		{ 0x121C,                          "icon0_27.png"              },
		{ 0x121D,                          "icon0_28.png"              },
		{ 0x121E,                          "icon0_29.png"              },
		{ 0x121F,                          "icon0_30.png"              },
		{ 0x1220,                          "pic0.png"                  },
		{ 0x1240,                          "snd0.at9"                  },
		{ 0x1260,                          "changeinfo/changeinfo.xml" },
		{ 0x1280,                          "icon0.dds"                 },
		{ 0x12A0,                          "pic0.dds"                  },
		{ 0x12C0,                          "pic1.dds"                  },
		{ 0x1400,                          "trophy/trophy00.trp"       }
		};
		std::string entry_name="";

		for (size_t i = 0; i < sizeof entries / sizeof entries[0]; i++) {
			if (type == entries[i].type) {
				entry_name = entries[i].name;
				break;
			}
		}

		return entry_name;
	}

};

