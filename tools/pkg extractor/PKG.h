#pragma once
#include <string>
#include "Types.h"
#include <io.h>
#include <windows.h>
#include <stdio.h>
#include <inttypes.h>

enum PKGFlags : U32 {
	PKG_FLAGS_UNKNOWN = 0x01,
	PKG_FLAGS_VER_1 = 0x01000000,
	PKG_FLAGS_VER_2 = 0x02000000,
	PKG_FLAGS_INTERNAL = 0x40000000,
	PKG_FLAGS_FINALIZED = 0x80000000,
};

enum PKGDrmType : U32
{
	PKG_DRM_TYPE_NONE = 0x0,
	PKG_DRM_TYPE_PS4 = 0xF,
};

enum PKGContentType : U32
{
	/* pkg_ps4_app, pkg_ps4_patch, pkg_ps4_remaster */
	PKG_CONTENT_TYPE_GD = 0x1A,
	/* pkg_ps4_ac_data, pkg_ps4_sf_theme, pkg_ps4_theme */
	PKG_CONTENT_TYPE_AC = 0x1B,
	/* pkg_ps4_ac_nodata */
	PKG_CONTENT_TYPE_AL = 0x1C,
	/* pkg_ps4_delta_patch */
	PKG_CONTENT_TYPE_DP = 0x1E,
};

enum PKGContentFlags : U32
{
	PKG_CONTENT_FLAGS_FIRST_PATCH = 0x00100000,
	PKG_CONTENT_FLAGS_PATCHGO = 0x00200000,
	PKG_CONTENT_FLAGS_REMASTER = 0x00400000,
	PKG_CONTENT_FLAGS_PS_CLOUD = 0x00800000,
	PKG_CONTENT_FLAGS_GD_AC = 0x02000000,
	PKG_CONTENT_FLAGS_NON_GAME = 0x04000000,
	PKG_CONTENT_FLAGS_Unk_x8000000 = 0x08000000,
	PKG_CONTENT_FLAGS_SUBSEQUENT_PATCH = 0x40000000,
	PKG_CONTENT_FLAGS_DELTA_PATCH = 0x41000000,
	PKG_CONTENT_FLAGS_CUMULATIVE_PATCH = 0x60000000,
};
enum IROTag : U32
{
	PKG_IRO_TAG_NONE = 0,
	/* SHAREfactory theme */
	PKG_IRO_TAG_SF_THEME = 0x1,
	/* System Software theme */
	PKG_IRO_TAG_SS_THEME = 0x2,
};


struct PKGHeader {
	/*BE*/U32 magic;// Magic
	/*BE*/U32 pkg_flags;
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
	ReadBE(s.pkg_flags);
	ReadBE(s.pkg_0x8);
	ReadBE(s.pkg_file_count);
	ReadBE(s.pkg_table_entry_count);
	ReadBE(s.pkg_sc_entry_count);
	ReadBE(s.pkg_table_entry_count_2);
	ReadBE(s.pkg_table_entry_offset);
	ReadBE(s.pkg_sc_entry_data_size);
	ReadBE(s.pkg_body_offset);
	ReadBE(s.pkg_body_size);
	ReadBE(s.pkg_content_offset);
	ReadBE(s.pkg_content_size);
	ReadBE(s.pkg_drm_type);
	ReadBE(s.pkg_content_type);
	ReadBE(s.pkg_content_flags);
	ReadBE(s.pkg_promote_size);
	ReadBE(s.pkg_version_date);
	ReadBE(s.pkg_version_hash);
	ReadBE(s.pkg_0x088);
	ReadBE(s.pkg_0x08C);
	ReadBE(s.pkg_0x090);
	ReadBE(s.pkg_0x094);
	ReadBE(s.pkg_iro_tag);
	ReadBE(s.pkg_drm_type_version);
	ReadBE(s.pkg_0x400);
	ReadBE(s.pfs_image_count);
	ReadBE(s.pfs_image_flags);
	ReadBE(s.pfs_image_offset);
	ReadBE(s.pfs_image_size);
	ReadBE(s.mount_image_offset);
	ReadBE(s.mount_image_size);
	ReadBE(s.pfs_signed_size);
	ReadBE(s.pfs_cache_size);
	ReadBE(s.pkg_size);
	ReadBE(s.pfs_split_size_nth_0);
	ReadBE(s.pfs_split_size_nth_1);
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
	PKGHeader pkgheader;

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
		void* pStart=nullptr;
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
		{ 0x0409,                          "psreserved.dat"            },
		{ 0x1000,                          "param.sfo"                 },
		{ 0x1001,                          "playgo-chunk.dat"          },
		{ 0x1002,                          "playgo-chunk.sha"          },
		{ 0x1003,                          "playgo-manifest.xml"       },
		{ 0x1004,                          "pronunciation.xml"         },
		{ 0x1005,                          "pronunciation.sig"         },
		{ 0x1006,                          "pic1.png"                  },
		{ 0x1007,                          "pubtoolinfo.dat"           },
		{ 0x100B,                          "shareparam.json"           },
		{ 0x100C,                          "shareoverlayimage.png"     },
		{ 0x100E,                          "shareprivacyguardimage.png"},
		{ 0x1200,                          "icon0.png"                 },
		{ 0x1220,                          "pic0.png"                  },
		{ 0x1240,                          "snd0.at9"                  },
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

	void printPkgHeader()
	{
		printf("PS4 PKG header:\n");
		printf("- PKG magic: 0x%X\n", pkgheader.magic);
		printf("- PKG flags: 0x%X", pkgheader.pkg_flags);
		if (pkgheader.pkg_flags & PKG_FLAGS_UNKNOWN)
			printf(" unknown");
		if (pkgheader.pkg_flags & PKG_FLAGS_VER_1)
			printf(" ver_1");
		if (pkgheader.pkg_flags & PKG_FLAGS_VER_2)
			printf(" ver_2");
		if (pkgheader.pkg_flags & PKG_FLAGS_INTERNAL)
			printf(" internal");
		if(pkgheader.pkg_flags & PKG_FLAGS_FINALIZED)
			printf(" finalized");
		printf("\n");
		printf("- PKG 0x8: 0x%X\n", pkgheader.pkg_0x8);
		printf("- PKG file count: %u\n", pkgheader.pkg_file_count);
		printf("- PKG table entries count:  %u\n", pkgheader.pkg_table_entry_count);
		printf("- PKG system entries count:  %u\n", pkgheader.pkg_sc_entry_count);
		printf("- PKG table entries2 count:  %u\n", pkgheader.pkg_table_entry_count_2);
		printf("- PKG table offset: 0x%X\n", pkgheader.pkg_table_entry_offset);
		printf("- PKG table entry data size: 0x%X\n", pkgheader.pkg_sc_entry_data_size);
		printf("- PKG body offset: 0x%" PRIx64 "\n", pkgheader.pkg_body_offset);
		printf("- PKG body size: 0x%" PRIx64 "\n", pkgheader.pkg_body_size);
		printf("- PKG content offset: 0x%" PRIx64 "\n", pkgheader.pkg_content_offset);
		printf("- PKG content size: 0x%" PRIx64 "\n", pkgheader.pkg_content_offset);
		printf("- PKG pkg_content_id: %s\n", pkgheader.pkg_content_id);

		printf("- PKG drm type: 0x%X", pkgheader.pkg_drm_type);
		if (pkgheader.pkg_drm_type == PKG_DRM_TYPE_NONE)
			printf(" None");
		if (pkgheader.pkg_drm_type == PKG_DRM_TYPE_PS4)
			printf(" PS4");
		printf("\n");
		printf("- PKG content type: 0x%X", pkgheader.pkg_content_type);
		if (pkgheader.pkg_content_type == PKG_CONTENT_TYPE_GD)
			printf(" GD");
		if (pkgheader.pkg_content_type == PKG_CONTENT_TYPE_AC)
			printf(" AC");
		if (pkgheader.pkg_content_type == PKG_CONTENT_TYPE_AL)
			printf(" AL");
		if (pkgheader.pkg_content_type == PKG_CONTENT_TYPE_DP)
			printf(" DP");
		printf("\n");
		printf("- PKG content flags: 0x%X", pkgheader.pkg_content_flags);
		if (pkgheader.pkg_content_flags & PKG_CONTENT_FLAGS_FIRST_PATCH)
			printf(" First_patch");
		if (pkgheader.pkg_content_flags & PKG_CONTENT_FLAGS_PATCHGO)
			printf(" Patch_go");
		if (pkgheader.pkg_content_flags & PKG_CONTENT_FLAGS_REMASTER)
			printf(" Remastered");
		if (pkgheader.pkg_content_flags & PKG_CONTENT_FLAGS_PS_CLOUD)
			printf(" Cloud");
		if (pkgheader.pkg_content_flags & PKG_CONTENT_FLAGS_GD_AC)
			printf(" AC");
		if (pkgheader.pkg_content_flags & PKG_CONTENT_FLAGS_NON_GAME)
			printf(" Non_game");
		if (pkgheader.pkg_content_flags & PKG_CONTENT_FLAGS_Unk_x8000000)
			printf(" Unk_x8000000");
		if (pkgheader.pkg_content_flags & PKG_CONTENT_FLAGS_SUBSEQUENT_PATCH)
			printf(" Subsequent_patch");
		if (pkgheader.pkg_content_flags & PKG_CONTENT_FLAGS_DELTA_PATCH)
			printf(" Delta_patch");
		if (pkgheader.pkg_content_flags & PKG_CONTENT_FLAGS_CUMULATIVE_PATCH)
			printf(" Cumulative_patch");
		printf("\n");
		printf("- PKG promote size: 0x%X\n", pkgheader.pkg_promote_size);
		printf("- PKG version date: 0x%X\n", pkgheader.pkg_version_date);
		printf("- PKG version hash: 0x%X\n", pkgheader.pkg_version_hash);
		printf("- PKG 0x088: 0x%X\n", pkgheader.pkg_0x088);
		printf("- PKG 0x08C: 0x%X\n", pkgheader.pkg_0x08C);
		printf("- PKG 0x090: 0x%X\n", pkgheader.pkg_0x090);
		printf("- PKG 0x094: 0x%X\n", pkgheader.pkg_0x094);
		printf("- PKG iro tag: 0x%X", pkgheader.pkg_iro_tag);
		if (pkgheader.pkg_iro_tag == PKG_IRO_TAG_NONE)
			printf(" None");
		if (pkgheader.pkg_iro_tag == PKG_IRO_TAG_SF_THEME)
			printf(" SF Theme");
		if (pkgheader.pkg_iro_tag == PKG_IRO_TAG_SS_THEME)
			printf(" SS Theme");
		printf("\n");
		printf("- PKG drm type version: 0x%X\n", pkgheader.pkg_drm_type_version);

		printf("- PKG digest_entries1: ");
		for (U08 s = 0; s < 0x20; s++) printf("%X", pkgheader.digest_entries1[s]);
		printf("\n");
		printf("- PKG digest_entries2: ");
		for (U08 s = 0; s < 0x20; s++)printf("%X", pkgheader.digest_entries2[s]);
		printf("\n");
		printf("- PKG digest_table_digest: ");
		for (U08 s = 0; s < 0x20; s++)printf("%X", pkgheader.digest_table_digest[s]);
		printf("\n");
		printf("- PKG digest_body_digest: ");
		for (U08 s = 0; s < 0x20; s++)printf("%X", pkgheader.digest_body_digest[s]);
		printf("\n");
		printf("- PKG 0x400: 0x%X\n", pkgheader.pkg_0x400);
		printf("- PKG pfs_image_count: 0x%X\n", pkgheader.pfs_image_count);
		printf("- PKG pfs_image_flags: 0x%" PRIx64 "\n", pkgheader.pfs_image_flags);
		printf("- PKG pfs_image_offset: 0x%" PRIx64 "\n", pkgheader.pfs_image_offset);
		printf("- PKG mount_image_offset: 0x%" PRIx64 "\n", pkgheader.mount_image_offset);
		printf("- PKG mount_image_size: 0x%" PRIx64 "\n", pkgheader.mount_image_size);
		printf("- PKG size: 0x%" PRIx64 "\n", pkgheader.pkg_size);
		printf("- PKG pfs_signed_size: 0x%X\n", pkgheader.pfs_signed_size);
		printf("- PKG pfs_cache_size: 0x%X\n", pkgheader.pfs_cache_size);
		printf("- PKG pfs_image_digest: ");
		for (U08 s = 0; s < 0x20; s++)printf("%X", pkgheader.pfs_image_digest[s]);
		printf("\n");
		printf("- PKG pfs_signed_digest: ");
		for (U08 s = 0; s < 0x20; s++)printf("%X", pkgheader.pfs_signed_digest[s]);
		printf("\n");
		printf("- PKG pfs_split_size_nth_0: 0x%" PRIx64 "\n", pkgheader.pfs_split_size_nth_0);
		printf("- PKG pfs_split_size_nth_1: 0x%" PRIx64 "\n", pkgheader.pfs_split_size_nth_1);
		printf("- PKG pkg_digest: ");
		for (U08 s = 0; s < 0x20; s++)printf("%X", pkgheader.pkg_digest[s]);
		printf("\n");
		printf("\n\n");
	}

	void printPkgFileEntry(PKGEntry entry, std::string name)
	{

		printf("-PS4 File Entry:\n");
		printf("--found name: %s\n", name.c_str());
		printf("--id: 0x%X\n", entry.id);		
		printf("--filename_offset: 0x%X\n", entry.filename_offset);
		printf("--flags1: 0x%X\n", entry.flags1);
		printf("--flags2: 0x%X\n", entry.flags2);
		printf("--offset: 0x%X\n", entry.offset);
		printf("--size: 0x%X\n", entry.size);
	}
};

