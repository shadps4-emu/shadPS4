#pragma once
#include <string>
#include "Types.h"
#include <io.h>
#include <windows.h>
#include <stdio.h>
#include <inttypes.h>

enum PFSMode : U08 {
    PFS_MODE_NONE = 0,
    PFS_MODE_SIGNED = 0x1,
    PFS_MODE_64BIT = 0x2,
    PFS_MODE_ENCRYPTED = 0x4,
    PFS_MODE_UNKNOWN = 0x8,
};
struct PFS_HDR {
    U64 version;
    U64 magic;
    U64 id;
    U08 fmode;
    U08 clean;
    U08 ronly;
    U08 rsv;
    U16 mode;
    U16 unk1;
    U32 blocksz;
    U32 nbackup;
    U64 nblock;
    U64 ndinode;
    U64 ndblock;
    U64 ndinodeblock;
    U64 superroot_ino;
};

class PFS
{
private:
    PFS_HDR psfOuterheader;
public:
    PFS();
    ~PFS();
    bool pfsOuterReadHeader(U08* psfOuterStart);

    void printPsfOuterHeader()
    {
        printf("PS4 PSF Outer Header:\n");
        printf("- version: 0x%" PRIx64 "\n", psfOuterheader.version);
        printf("- magic: 0x%" PRIx64 "\n", psfOuterheader.magic);
        printf("- id: 0x%" PRIx64 "\n", psfOuterheader.id);
        printf("- fmode: 0x%X\n", psfOuterheader.fmode);
        printf("- clean: 0x%X\n", psfOuterheader.clean);
        printf("- ronly: 0x%X\n", psfOuterheader.ronly);
        printf("- rsv: 0x%X\n", psfOuterheader.rsv);
        printf("- mode: 0x%X", psfOuterheader.mode);
        if (psfOuterheader.mode & PFS_MODE_SIGNED)
            printf(" signed");
        if (psfOuterheader.mode & PFS_MODE_64BIT)
            printf(" 64-bit");
        if (psfOuterheader.mode & PFS_MODE_ENCRYPTED)
            printf(" encrypted");
        if (psfOuterheader.mode & PFS_MODE_UNKNOWN)
            printf(" unknown");
        printf("\n");
        printf("- unk1: 0x%X\n", psfOuterheader.unk1);
        printf("- blocksz: 0x%X\n", psfOuterheader.blocksz);
        printf("- nbackup: 0x%X\n", psfOuterheader.nbackup);
        printf("- nblock: 0x%" PRIx64 "\n", psfOuterheader.nblock);
        printf("- ndinode: 0x%" PRIx64 "\n", psfOuterheader.ndinode);
        printf("- ndblock: 0x%" PRIx64 "\n", psfOuterheader.ndblock);
        printf("- ndinodeblock: 0x%" PRIx64 "\n", psfOuterheader.ndinodeblock);
        printf("- superroot_ino: 0x%" PRIx64 "\n", psfOuterheader.superroot_ino);

        printf("\n\n");
    }
};