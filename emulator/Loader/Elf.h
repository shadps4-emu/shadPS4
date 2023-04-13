#pragma once
#include <string>
#include <inttypes.h>
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
    u64 file_offset;
    u64 file_size;
    u64 memory_size;
};


constexpr u08 EI_MAG0 = 0;/* e_ident[] indexes */
constexpr u08 EI_MAG1 = 1;
constexpr u08 EI_MAG2 = 2;
constexpr u08 EI_MAG3 = 3;
constexpr u08 EI_CLASS = 4;
constexpr u08 EI_DATA = 5;
constexpr u08 EI_VERSION = 6;
constexpr u08 EI_OSABI = 7;
constexpr u08 EI_ABIVERSION = 8;

// Magic number
constexpr u08 ELFMAG0 = 0x7F;
constexpr u08 ELFMAG1 = 'E';
constexpr u08 ELFMAG2 = 'L';
constexpr u08 ELFMAG3 = 'F';

//other ident fields , only ps4 neccesary ones
constexpr u08 ELFCLASS64 = 2;
constexpr u08 ELFDATA2LSB = 1;
constexpr u08 ELFOSABI_FREEBSD = 9;  // FreeBSD
constexpr u08 EV_CURRENT = 1;
constexpr u08 ELFABIVERSION_AMDGPU_HSA_V2 = 0;

//type fields PS4 specific
constexpr u16 ET_DYNEXEC = 0xFE10; // Executable file
constexpr u16 ET_DYNAMIC = 0xFE18; // Shared

//machine field
constexpr u16 EM_X86_64 = 62; // Advanced Micro Devices X86-64 processor

struct elf_header
{
    u08 e_ident[16];        /* ELF identification */
    u16 e_type;             /* Object file type */
    u16 e_machine;          /* Machine type */
    u32 e_version;          /* Object file version */
    u64 e_entry;            /* Entry point address */
    u64 e_phoff;            /* Program header offset */
    u64 e_shoff;            /* Section header offset */
    u32 e_flags;            /* Processor-specific flags */
    u16 e_ehsize;           /* ELF header size */
    u16 e_phentsize;        /* Size of program header entry */
    u16 e_phnum;            /* Number of program header entries */
    u16 e_shentsize;        /* Size of section header entry */
    u16 e_shnum;            /* Number of section header entries */
    u16 e_shstrndx;         /* Section name string table index */
};

struct elf_program_header 
{
    u32 p_type;   /* Type of segment */
    u32 p_flags;  /* Segment attributes */
    u64 p_offset; /* Offset in file */
    u64 p_vaddr;  /* Virtual address in memory */
    u64 p_paddr;  /* Reserved */
    u64 p_filesz; /* Size of segment in file */
    u64 p_memsz;  /* Size of segment in memory */
    u64 p_align;  /* Alignment of segment */
};

struct elf_section_header
{
    u32 sh_name;      /* Section name */
    u32 sh_type;      /* Section type */
    u64 sh_flags;     /* Section attributes */
    u64 sh_addr;      /* Virtual address in memory */
    u64 sh_offset;    /* Offset in file */
    u64 sh_size;      /* Size of section */
    u32 sh_link;      /* Link to other section */
    u32 sh_info;      /* Miscellaneous information */
    u64 sh_addralign; /* Address alignment boundary */
    u64 sh_entsize;   /* Size of entries, if section has table */
};

struct elf_program_id_header
{
    u64 authid;
    u64 program_type;
    u64 appver;
    u64 firmver;
    u08 digest[32];
};
class Elf
{
public:
    Elf() = default;
    virtual ~Elf();

    void Open(const std::string & file_name);
    bool isSelfFile() const;
    bool isElfFile() const;
    void DebugDump();
private:

    void Reset();

    FsFile* m_f = nullptr;
    self_header* m_self = nullptr;
    self_segment_header* m_self_segments = nullptr;
    elf_header* m_elf_header = nullptr;
    elf_program_header* m_elf_phdr = nullptr;
    elf_section_header* m_elf_shdr = nullptr;
    elf_program_id_header* m_self_id_header = nullptr;
};

