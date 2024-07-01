// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <string>
#include <vector>

#include "common/io_file.h"
#include "common/types.h"

struct self_header {
    static constexpr u32 signature = 0x1D3D154Fu;

    u32 magic;
    u8 version;
    u8 mode;
    u8 endian; // 1 is little endian
    u8 attributes;
    u8 category;
    u8 program_type;
    u16 padding1;
    u16 header_size;
    u16 meta_size;
    u32 file_size;
    u32 padding2;
    u16 segment_count;
    u16 unknown1A; // always 0x22
    u32 padding3;
};

struct self_segment_header {
    bool IsBlocked() const {
        return (flags & 0x800) != 0; // 0 or 0x800
    }

    u32 GetId() const {
        return (flags >> 20u) & 0xFFFu;
    }

    bool IsOrdered() const {
        return (flags & 1) != 0; // 0 or 1
    }

    bool IsEncrypted() const {
        return (flags & 2) != 0; // 0 or 2
    }

    bool IsSigned() const {
        return (flags & 4) != 0; // 0 or 4
    }

    bool IsCompressed() const {
        return (flags & 8) != 0; // 0 or 8
    }

    u64 flags;
    u64 file_offset;
    u64 file_size;
    u64 memory_size;
};

constexpr u8 EI_MAG0 = 0; /* e_ident[] indexes */
constexpr u8 EI_MAG1 = 1;
constexpr u8 EI_MAG2 = 2;
constexpr u8 EI_MAG3 = 3;
constexpr u8 EI_CLASS = 4;
constexpr u8 EI_DATA = 5;
constexpr u8 EI_VERSION = 6;
constexpr u8 EI_OSABI = 7;
constexpr u8 EI_ABIVERSION = 8;

// Magic number
constexpr u8 ELFMAG0 = 0x7F;
constexpr u8 ELFMAG1 = 'E';
constexpr u8 ELFMAG2 = 'L';
constexpr u8 ELFMAG3 = 'F';

typedef enum : u16 {
    ET_NONE = 0x0,
    ET_REL = 0x1,
    ET_EXEC = 0x2,
    ET_DYN = 0x3,
    ET_CORE = 0x4,
    ET_SCE_EXEC = 0xfe00,
    ET_SCE_STUBLIB = 0xfe0c,
    ET_SCE_DYNEXEC = 0xfe10,
    ET_SCE_DYNAMIC = 0xfe18
} e_type_s;

typedef enum : u16 {
    EM_NONE = 0,         /* No machine */
    EM_M32 = 1,          /* AT&T WE 32100 */
    EM_SPARC = 2,        /* SPARC */
    EM_386 = 3,          /* Intel 80386 */
    EM_68K = 4,          /* Motorola 68000 */
    EM_88K = 5,          /* Motorola 88000 */
    EM_860 = 7,          /* Intel 80860 */
    EM_MIPS = 8,         /* MIPS I Architecture */
    EM_S370 = 9,         /* IBM System/370 Processor */
    EM_MIPS_RS3_LE = 10, /* MIPS RS3000 Little-endian */
    EM_PARISC = 15,      /* Hewlett-Packard PA-RISC */
    EM_VPP500 = 17,      /* Fujitsu VPP500 */
    EM_SPARC32PLUS = 18, /* Enhanced instruction set SPARC */
    EM_960 = 19,         /* Intel 80960 */
    EM_PPC = 20,         /* PowerPC */
    EM_PPC64 = 21,       /* 64-bit PowerPC */
    EM_S390 = 22,        /* IBM System/390 Processor */
    EM_V800 = 36,        /* NEC V800 */
    EM_FR20 = 37,        /* Fujitsu FR20 */
    EM_RH32 = 38,        /* TRW RH-32 */
    EM_RCE = 39,         /* Motorola RCE */
    EM_ARM = 40,         /* Advanced RISC Machines ARM */
    EM_ALPHA = 41,       /* Digital Alpha */
    EM_SH = 42,          /* Hitachi SH */
    EM_SPARCV9 = 43,     /* SPARC Version 9 */
    EM_TRICORE = 44,     /* Siemens TriCore embedded processor */
    EM_ARC = 45,         /* Argonaut RISC Core, Argonaut Technologies Inc. */
    EM_H8_300 = 46,      /* Hitachi H8/300 */
    EM_H8_300H = 47,     /* Hitachi H8/300H */
    EM_H8S = 48,         /* Hitachi H8S */
    EM_H8_500 = 49,      /* Hitachi H8/500 */
    EM_IA_64 = 50,       /* Intel IA-64 processor architecture */
    EM_MIPS_X = 51,      /* Stanford MIPS-X */
    EM_COLDFIRE = 52,    /* Motorola ColdFire */
    EM_68HC12 = 53,      /* Motorola M68HC12 */
    EM_MMA = 54,         /* Fujitsu MMA Multimedia Accelerator */
    EM_PCP = 55,         /* Siemens PCP */
    EM_NCPU = 56,        /* Sony nCPU embedded RISC processor */
    EM_NDR1 = 57,        /* Denso NDR1 microprocessor */
    EM_STARCORE = 58,    /* Motorola Star*Core processor */
    EM_ME16 = 59,        /* Toyota ME16 processor */
    EM_ST100 = 60,       /* STMicroelectronics ST100 processor */
    EM_TINYJ = 61,       /* Advanced Logic Corp. TinyJ embedded processor family */
    EM_X86_64 = 62,      /* AMD x86-64 architecture (PS4) */
    EM_PDSP = 63,        /* Sony DSP Processor */
    EM_PDP10 = 64,       /* Digital Equipment Corp. PDP-10 */
    EM_PDP11 = 65,       /* Digital Equipment Corp. PDP-11 */
    EM_FX66 = 66,        /* Siemens FX66 microcontroller */
    EM_ST9PLUS = 67,     /* STMicroelectronics ST9+ 8/16 bit microcontroller */
    EM_ST7 = 68,         /* STMicroelectronics ST7 8-bit microcontroller */
    EM_68HC16 = 69,      /* Motorola MC68HC16 Microcontroller */
    EM_68HC11 = 70,      /* Motorola MC68HC11 Microcontroller */
    EM_68HC08 = 71,      /* Motorola MC68HC08 Microcontroller */
    EM_68HC05 = 72,      /* Motorola MC68HC05 Microcontroller */
    EM_SVX = 73,         /* Silicon Graphics SVx */
    EM_ST19 = 75,        /* Digital VAX */
    EM_CRIS = 76,        /* Axis Communications 32-bit embedded processor */
    EM_JAVELIN = 77,     /* Infineon Technologies 32-bit embedded processor */
    EM_FIREPATH = 78,    /* Element 14 64-bit DSP Processor */
    EM_ZSP = 79,         /* LSI Logic 16-bit DSP Processor */
    EM_MMIX = 80,        /* Donald Knuth's educational 64-bit processor */
    EM_HUANY = 81,       /* Harvard University machine-independent object files */
    EM_PRISM = 82,       /* SiTera Prism */
    EM_AVR = 83,         /* Atmel AVR 8-bit microcontroller */
    EM_FR30 = 84,        /* Fujitsu FR30 */
    EM_D10V = 85,        /* Mitsubishi D10V */
    EM_D30V = 86,        /* Mitsubishi D30V */
    EM_V850 = 87,        /* NEC v850 */
    EM_M32R = 88,        /* Mitsubishi M32R */
    EM_MN10300 = 89,     /* Matsushita MN10300 */
    EM_MN10200 = 90,     /* Matsushita MN10200 */
    EM_PJ = 91,          /* PicoJava */
    EM_OPENRISC = 92,    /* OpenRISC 32-bit embedded processor */
    EM_ARC_A5 = 93,      /* ARC Cores Tangent-A5 */
    EM_XTENSA = 94,      /* Tensilica Xtensa Architecture */
    EM_VIDEOCORE = 95,   /* Alphamosaic VideoCore processor */
    EM_TMM_GPP = 96,     /* Thompson Multimedia General Purpose Processor */
    EM_NS32K = 97,       /* National Semiconductor 32000 series */
    EM_TPC = 98,         /* Tenor Network TPC processor */
    EM_SNP1K = 99,       /* Trebia SNP 1000 processor */
    EM_ST200 = 100,      /* STMicroelectronics (www.st.com) ST200 microcontroller */
    EM_IP2K = 101,       /* Ubicom IP2xxx microcontroller family */
    EM_MAX = 102,        /* MAX Processor */
    EM_CR = 103,         /* National Semiconductor CompactRISC microprocessor */
    EM_F2MC16 = 104,     /* Fujitsu F2MC16 */
    EM_MSP430 = 105,     /* Texas Instruments embedded microcontroller msp430 */
    EM_BLACKFIN = 106,   /* Analog Devices Blackfin (DSP) processor */
    EM_SE_C33 = 107,     /* S1C33 Family of Seiko Epson processors */
    EM_SEP = 108,        /* Sharp embedded microprocessor */
    EM_ARCA = 109,       /* Arca RISC Microprocessor */
    EM_UNICORE = 110     /* Microprocessor series from PKU-Unity Ltd. and MPRC */
} e_machine_es;

typedef enum : u32 { EV_NONE = 0x0, EV_CURRENT = 0x1 } e_version_es;

typedef enum : u8 {
    ELF_CLASS_NONE = 0x0,
    ELF_CLASS_32 = 0x1,
    ELF_CLASS_64 = 0x2,
    ELF_CLASS_NUM = 0x3
} ident_class_es;

typedef enum : u8 {
    ELF_DATA_NONE = 0x0,
    ELF_DATA_2LSB = 0x1,
    ELF_DATA_2MSB = 0x2,
    ELF_DATA_NUM = 0x3
} ident_endian_es;

typedef enum : u8 {
    ELF_VERSION_NONE = 0x0,
    ELF_VERSION_CURRENT = 0x1,
    ELF_VERSION_NUM = 0x2
} ident_version_es;

typedef enum : u8 {
    ELF_OSABI_NONE = 0x0,       /* No extensions or unspecified */
    ELF_OSABI_HPUX = 0x1,       /* Hewlett-Packard HP-UX */
    ELF_OSABI_NETBSD = 0x2,     /* NetBSD */
    ELF_OSABI_LINUX = 0x3,      /* Linux */
    ELF_OSABI_SOLARIS = 0x6,    /* Sun Solaris */
    ELF_OSABI_AIX = 0x7,        /* AIX */
    ELF_OSABI_IRIX = 0x8,       /* IRIX */
    ELF_OSABI_FREEBSD = 0x9,    /* FreeBSD (PS4) */
    ELF_OSABI_TRU64 = 0xA,      /* Compaq TRU64 UNIX */
    ELF_OSABI_MODESTO = 0xB,    /* Novell Modesto */
    ELF_OSABI_OPENBSD = 0xC,    /* Open BSD */
    ELF_OSABI_OPENVMS = 0xD,    /* Open VMS */
    ELF_OSABI_NSK = 0xE,        /* Hewlett-Packard Non-Stop Kernel */
    ELF_OSABI_AROS = 0xF,       /* Amiga Research OS */
    ELF_OSABI_ARM_AEABI = 0x40, /* ARM EABI */
    ELF_OSABI_ARM = 0x61,       /* ARM */
    ELF_OSABI_STANDALONE = 0xFF /* Standalone (embedded applications) */
} ident_osabi_es;

typedef enum : u8 {
    ELF_ABI_VERSION_AMDGPU_HSA_V2 = 0x0,
    ELF_ABI_VERSION_AMDGPU_HSA_V3 = 0x1,
    ELF_ABI_VERSION_AMDGPU_HSA_V4 = 0x2,
    ELF_ABI_VERSION_AMDGPU_HSA_V5 = 0x3
} ident_abiversion_es;

struct elf_ident {
    u8 magic[4];
    ident_class_es ei_class;
    ident_endian_es ei_data;
    ident_version_es ei_version;
    ident_osabi_es ei_osabi;
    ident_abiversion_es ei_abiversion;
    u8 pad[6];
};

struct elf_header {
    static const u32 signature = 0x7F454C46u;

    elf_ident e_ident;      /* ELF identification */
    e_type_s e_type;        /* Object file type */
    e_machine_es e_machine; /* Machine type */
    e_version_es e_version; /* Object file version */
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

typedef enum : u32 {
    PT_NULL = 0x0,
    PT_LOAD = 0x1,
    PT_DYNAMIC = 0x2,
    PT_INTERP = 0x3,
    PT_NOTE = 0x4,
    PT_SHLIB = 0x5,
    PT_PHDR = 0x6,
    PT_TLS = 0x7,
    PT_NUM = 0x8,
    PT_SCE_RELA = 0x60000000,
    PT_SCE_DYNLIBDATA = 0x61000000,
    PT_SCE_PROCPARAM = 0x61000001,
    PT_SCE_MODULE_PARAM = 0x61000002,
    PT_SCE_RELRO = 0x61000010,
    PT_GNU_EH_FRAME = 0x6474e550,
    PT_GNU_STACK = 0x6474e551,
    PT_GNU_RELRO = 0x6474e552,
    PT_SCE_COMMENT = 0x6fffff00,
    PT_SCE_LIBVERSION = 0x6fffff01,
    PT_LOSUNW = 0x6ffffffa,
    PT_SUNWBSS = 0x6ffffffa,
    PT_SUNWSTACK = 0x6ffffffb,
    PT_HISUNW = 0x6fffffff,
    PT_HIOS = 0x6fffffff,
    PT_LOPROC = 0x70000000,
    PT_HIPROC = 0x7fffffff
} elf_program_type;

typedef enum : u32 {
    PF_NONE = 0x0,
    PF_EXEC = 0x1,
    PF_WRITE = 0x2,
    PF_WRITE_EXEC = 0x3,
    PF_READ = 0x4,
    PF_READ_EXEC = 0x5,
    PF_READ_WRITE = 0x6,
    PF_READ_WRITE_EXEC = 0x7
} elf_program_flags;

struct elf_program_header {
    elf_program_type p_type;   /* Type of segment */
    elf_program_flags p_flags; /* Segment attributes */
    u64 p_offset;              /* Offset in file */
    u64 p_vaddr;               /* Virtual address in memory */
    u64 p_paddr;               /* Reserved */
    u64 p_filesz;              /* Size of segment in file */
    u64 p_memsz;               /* Size of segment in memory */
    u64 p_align;               /* Alignment of segment */
};

struct elf_section_header {
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

typedef enum : u64 {
    PT_FAKE = 0x1,
    PT_NPDRM_EXEC = 0x4,
    PT_NPDRM_DYNLIB = 0x5,
    PT_SYSTEM_EXEC = 0x8,
    PT_SYSTEM_DYNLIB = 0x9,
    PT_HOST_KERNEL = 0xC,
    PT_SECURE_MODULE = 0xE,
    PT_SECURE_KERNEL = 0xF
} program_type_es;

struct elf_program_id_header {
    u64 authid;
    program_type_es program_type;
    u64 appver;
    u64 firmver;
    u8 digest[32];
};

constexpr s64 DT_NULL = 0;
constexpr s64 DT_NEEDED = 0x00000001;
constexpr s64 DT_RELA = 0x00000007;
constexpr s64 DT_INIT = 0x0000000c;
constexpr s64 DT_FINI = 0x0000000d;
constexpr s64 DT_DEBUG = 0x00000015;
constexpr s64 DT_TEXTREL = 0x00000016;
constexpr s64 DT_INIT_ARRAY = 0x00000019;
constexpr s64 DT_FINI_ARRAY = 0x0000001a;
constexpr s64 DT_INIT_ARRAYSZ = 0x0000001b;
constexpr s64 DT_FINI_ARRAYSZ = 0x0000001c;
constexpr s64 DT_FLAGS = 0x0000001e;
constexpr s64 DT_PREINIT_ARRAY = 0x00000020;
constexpr s64 DT_PREINIT_ARRAYSZ = 0x00000021;
constexpr s64 DT_SCE_FINGERPRINT = 0x61000007;
constexpr s64 DT_SCE_ORIGINAL_FILENAME = 0x61000009;
constexpr s64 DT_SCE_MODULE_INFO = 0x6100000d;
constexpr s64 DT_SCE_NEEDED_MODULE = 0x6100000f;
constexpr s64 DT_SCE_MODULE_ATTR = 0x61000011;
constexpr s64 DT_SCE_EXPORT_LIB = 0x61000013;
constexpr s64 DT_SCE_IMPORT_LIB = 0x61000015;
constexpr s64 DT_SCE_IMPORT_LIB_ATTR = 0x61000019;
constexpr s64 DT_SCE_HASH = 0x61000025;
constexpr s64 DT_SCE_PLTGOT = 0x61000027;
constexpr s64 DT_SCE_JMPREL = 0x61000029;
constexpr s64 DT_SCE_PLTREL = 0x6100002b;
constexpr s64 DT_SCE_PLTRELSZ = 0x6100002d;
constexpr s64 DT_SCE_RELA = 0x6100002f;
constexpr s64 DT_SCE_RELASZ = 0x61000031;
constexpr s64 DT_SCE_RELAENT = 0x61000033;
constexpr s64 DT_SCE_SYMENT = 0x6100003b;
constexpr s64 DT_SCE_HASHSZ = 0x6100003d;
constexpr s64 DT_SCE_STRTAB = 0x61000035;
constexpr s64 DT_SCE_STRSZ = 0x61000037;
constexpr s64 DT_SCE_SYMTAB = 0x61000039;
constexpr s64 DT_SCE_SYMTABSZ = 0x6100003f;

struct elf_dynamic {
    s64 d_tag;
    union {
        u64 d_val;
        u64 d_ptr;
    } d_un;
};

constexpr u8 STB_LOCAL = 0;
constexpr u8 STB_GLOBAL = 1;
constexpr u8 STB_WEAK = 2;

constexpr u8 STT_NOTYPE = 0;
constexpr u8 STT_OBJECT = 1;
constexpr u8 STT_FUN = 2;
constexpr u8 STT_SECTION = 3;
constexpr u8 STT_FILE = 4;
constexpr u8 STT_COMMON = 5;
constexpr u8 STT_TLS = 6;
constexpr u8 STT_LOOS = 10;
constexpr u8 STT_SCE = 11; // module_start/module_stop
constexpr u8 STT_HIOS = 12;
constexpr u8 STT_LOPRO = 13;
constexpr u8 STT_SPARC_REGISTER = 13;
constexpr u8 STT_HIPROC = 15;

constexpr u8 STV_DEFAULT = 0;
constexpr u8 STV_INTERNAL = 1;
constexpr u8 STV_HIDDEN = 2;
constexpr u8 STV_PROTECTED = 3;

struct elf_symbol {
    u8 GetBind() const {
        return st_info >> 4u;
    }
    u8 GetType() const {
        return st_info & 0xfu;
    }
    u8 GetVisibility() const {
        return st_other & 3u;
    }

    u32 st_name;
    u8 st_info;
    u8 st_other;
    u16 st_shndx;
    u64 st_value;
    u64 st_size;
};

struct elf_relocation {
    u32 GetSymbol() const {
        return static_cast<u32>(rel_info >> 32u);
    }
    u32 GetType() const {
        return static_cast<u32>(rel_info & 0xffffffff);
    }

    u64 rel_offset;
    u64 rel_info;
    s64 rel_addend;
};
constexpr u32 R_X86_64_64 = 1; // Direct 64 bit
constexpr u32 R_X86_64_GLOB_DAT = 6;
constexpr u32 R_X86_64_JUMP_SLOT = 7; // Create PLT entry
constexpr u32 R_X86_64_RELATIVE = 8;  // Adjust by program base
constexpr u32 R_X86_64_DTPMOD64 = 16;

struct eh_frame_hdr {
    uint8_t version;
    uint8_t eh_frame_ptr_enc;
    uint8_t fde_count_enc;
    uint8_t table_enc;
    uint32_t eh_frame_ptr;
    uint32_t fde_count;
};

namespace Core::Loader {

class Elf {
public:
    Elf() = default;
    ~Elf();

    void Open(const std::filesystem::path& file_name);
    bool IsSelfFile() const;
    bool IsElfFile() const;

    [[nodiscard]] self_header GetSElfHeader() const {
        return m_self;
    }

    [[nodiscard]] elf_header GetElfHeader() const {
        return m_elf_header;
    }

    [[nodiscard]] std::span<const elf_program_header> GetProgramHeader() const {
        return m_elf_phdr;
    }

    [[nodiscard]] std::span<const self_segment_header> GetSegmentHeader() const {
        return m_self_segments;
    }

    [[nodiscard]] u64 GetElfEntry() const {
        return m_elf_header.e_entry;
    }

    [[nodiscard]] bool IsSharedLib() const {
        return m_elf_header.e_type == ET_SCE_DYNAMIC;
    }

    std::string SElfHeaderStr();
    std::string SELFSegHeader(u16 no);
    std::string ElfHeaderStr();
    std::string ElfPHeaderStr(u16 no);
    std::string_view ElfPheaderTypeStr(u32 type);
    std::string ElfPheaderFlagsStr(u32 flags);

    void LoadSegment(u64 virtual_addr, u64 file_offset, u64 size);
    bool IsSharedLib();
    void ElfHeaderDebugDump(const std::filesystem::path& file_name);
    void SelfHeaderDebugDump(const std::filesystem::path& file_name);
    void SelfSegHeaderDebugDump(const std::filesystem::path& file_name);
    void PHeaderDebugDump(const std::filesystem::path& file_name);

private:
    Common::FS::IOFile m_f{};
    bool is_self{};
    self_header m_self{};
    std::vector<self_segment_header> m_self_segments;
    elf_header m_elf_header{};
    std::vector<elf_program_header> m_elf_phdr;
    std::vector<elf_section_header> m_elf_shdr;
    elf_program_id_header m_self_id_header{};
};

} // namespace Core::Loader
