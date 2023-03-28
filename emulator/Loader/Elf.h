#pragma once
#include <string>
#include "../types.h"
#include "../Core/FsFile.h"

struct self_header
{
    static const u32 signature = 0x1D3D154Fu;

    u32 magic;
    u08 version;
    u08 mode;
    u08 endian;// 1 is little endian
    u08 attributes;
    u08 category;
    u08 program_type;
    u16 padding1;
    u16 header_size;
    u16 meta_size;
    u32 file_size;
    u32 padding2;
    u16 segment_count;
    u16 unknown1A; //always 0x22
    u32 padding3;
};

struct self_segment_header
{
    u64 flags;
    u64 offset;
    u64 encrypted_compressed_size;
    u64 decrypted_decompressed_size;
};

class Elf
{
public:
    void Open(const std::string & file_name);

private:
    FsFile* m_f = nullptr;
    self_header* m_self = nullptr;
};

