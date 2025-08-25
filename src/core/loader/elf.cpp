// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fmt/core.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/loader/elf.h"

namespace Core::Loader {

using namespace Common::FS;

static std::string_view GetProgramTypeName(program_type_es type) {
    switch (type) {
    case PT_FAKE:
        return "PT_FAKE";
    case PT_NPDRM_EXEC:
        return "PT_NPDRM_EXEC";
    case PT_NPDRM_DYNLIB:
        return "PT_NPDRM_DYNLIB";
    case PT_SYSTEM_EXEC:
        return "PT_SYSTEM_EXEC";
    case PT_SYSTEM_DYNLIB:
        return "PT_SYSTEM_DYNLIB";
    case PT_HOST_KERNEL:
        return "PT_HOST_KERNEL";
    case PT_SECURE_MODULE:
        return "PT_SECURE_MODULE";
    case PT_SECURE_KERNEL:
        return "PT_SECURE_KERNEL";
    default:
        return "INVALID";
    }
}

static std::string_view GetIdentClassName(ident_class_es elf_class) {
    switch (elf_class) {
    case ELF_CLASS_NONE:
        return "ELF_CLASS_NONE";
    case ELF_CLASS_32:
        return "ELF_CLASS_32";
    case ELF_CLASS_64:
        return "ELF_CLASS_64";
    case ELF_CLASS_NUM:
        return "ELF_CLASS_NUM";
    default:
        return "INVALID";
    }
}

static std::string_view GetIdentEndianName(ident_endian_es endian) {
    switch (endian) {
    case ELF_DATA_NONE:
        return "ELF_DATA_NONE";
    case ELF_DATA_2LSB:
        return "ELF_DATA_2LSB";
    case ELF_DATA_2MSB:
        return "ELF_DATA_2MSB";
    case ELF_DATA_NUM:
        return "ELF_DATA_NUM";
    default:
        return "INVALID";
    }
}

static std::string_view GetIdentVersionName(ident_version_es version) {
    switch (version) {
    case ELF_VERSION_NONE:
        return "ELF_VERSION_NONE";
    case ELF_VERSION_CURRENT:
        return "ELF_VERSION_CURRENT";
    case ELF_VERSION_NUM:
        return "ELF_VERSION_NUM";
    default:
        return "INVALID";
    }
}

static std::string_view GetIdentOsabiName(ident_osabi_es osabi) {
    switch (osabi) {
    case ELF_OSABI_NONE:
        return "ELF_OSABI_NONE";
    case ELF_OSABI_HPUX:
        return "ELF_OSABI_HPUX";
    case ELF_OSABI_NETBSD:
        return "ELF_OSABI_NETBSD";
    case ELF_OSABI_LINUX:
        return "ELF_OSABI_LINUX";
    case ELF_OSABI_SOLARIS:
        return "ELF_OSABI_SOLARIS";
    case ELF_OSABI_AIX:
        return "ELF_OSABI_AIX";
    case ELF_OSABI_IRIX:
        return "ELF_OSABI_IRIX";
    case ELF_OSABI_FREEBSD:
        return "ELF_OSABI_FREEBSD";
    case ELF_OSABI_TRU64:
        return "ELF_OSABI_TRU64";
    case ELF_OSABI_MODESTO:
        return "ELF_OSABI_MODESTO";
    case ELF_OSABI_OPENBSD:
        return "ELF_OSABI_OPENBSD";
    case ELF_OSABI_OPENVMS:
        return "ELF_OSABI_OPENVMS";
    case ELF_OSABI_NSK:
        return "ELF_OSABI_NSK";
    case ELF_OSABI_AROS:
        return "ELF_OSABI_AROS";
    case ELF_OSABI_ARM_AEABI:
        return "ELF_OSABI_ARM_AEABI";
    case ELF_OSABI_ARM:
        return "ELF_OSABI_ARM";
    case ELF_OSABI_STANDALONE:
        return "ELF_OSABI_STANDALONE";
    default:
        return "INVALID";
    }
}

static std::string_view GetIdentAbiversionName(ident_abiversion_es version) {
    switch (version) {
    case ELF_ABI_VERSION_AMDGPU_HSA_V2:
        return "ELF_ABI_VERSION_AMDGPU_HSA_V2";
    case ELF_ABI_VERSION_AMDGPU_HSA_V3:
        return "ELF_ABI_VERSION_AMDGPU_HSA_V3";
    case ELF_ABI_VERSION_AMDGPU_HSA_V4:
        return "ELF_ABI_VERSION_AMDGPU_HSA_V4";
    case ELF_ABI_VERSION_AMDGPU_HSA_V5:
        return "ELF_ABI_VERSION_AMDGPU_HSA_V5";
    default:
        return "INVALID";
    }
}
static std::string_view GetVersion(e_version_es version) {
    switch (version) {
    case EV_NONE:
        return "EV_NONE";
    case EV_CURRENT:
        return "EV_CURRENT";
    default:
        return "INVALID";
    }
}

static std::string_view GetType(e_type_s type) {
    switch (type) {
    case ET_NONE:
        return "ET_NONE";
    case ET_REL:
        return "ET_REL";
    case ET_EXEC:
        return "ET_EXEC";
    case ET_DYN:
        return "ET_DYN";
    case ET_CORE:
        return "ET_CORE";
    case ET_SCE_EXEC:
        return "ET_SCE_EXEC";
    case ET_SCE_STUBLIB:
        return "ET_SCE_STUBLIB";
    case ET_SCE_DYNEXEC:
        return "ET_SCE_DYNEXEC";
    case ET_SCE_DYNAMIC:
        return "ET_SCE_DYNAMIC";
    default:
        return "INVALID";
    }
}

static std::string_view GetMachine(e_machine_es machine) {
    switch (machine) {
    case EM_X86_64:
        return "EM_X86_64";
    default:
        return "INVALID";
    }
}

Elf::~Elf() = default;

void Elf::Open(const std::filesystem::path& file_name) {
    m_f.Open(file_name, FileAccessMode::Read);
    if (!m_f.ReadObject(m_self)) {
        LOG_ERROR(Loader, "Unable to read self header!");
        return;
    }

    if (is_self = IsSelfFile(); !is_self) {
        m_f.Seek(0, SeekOrigin::SetOrigin);
    } else {
        m_self_segments.resize(m_self.segment_count);
        m_f.Read(m_self_segments);
    }

    const u64 elf_header_pos = m_f.Tell();
    m_f.Read(m_elf_header);
    if (!IsElfFile()) {
        return;
    }

    const auto load_headers = [this]<typename T>(std::vector<T>& out, u64 offset, u16 num) {
        if (!num) {
            return;
        }

        out.resize(num);
        if (!m_f.Seek(offset, SeekOrigin::SetOrigin)) {
            LOG_CRITICAL(Loader, "Failed to seek to header tables");
            return;
        }
        m_f.Read(out);
    };

    load_headers(m_elf_phdr, elf_header_pos + m_elf_header.e_phoff, m_elf_header.e_phnum);
    load_headers(m_elf_shdr, elf_header_pos + m_elf_header.e_shoff, m_elf_header.e_shnum);

    if (is_self) {
        u64 header_size = 0;
        header_size += sizeof(self_header);
        header_size += sizeof(self_segment_header) * m_self.segment_count;
        header_size += sizeof(elf_header);
        header_size += m_elf_header.e_phnum * m_elf_header.e_phentsize;
        header_size += m_elf_header.e_shnum * m_elf_header.e_shentsize;
        header_size += 15;
        header_size &= ~15; // Align

        if (m_elf_header.e_ehsize - header_size >= sizeof(elf_program_id_header)) {
            m_f.Seek(header_size, SeekOrigin::SetOrigin);
            m_f.ReadObject(m_self_id_header);
        }
    }
}

bool Elf::IsSelfFile() const {
    if (m_self.magic != self_header::signature) [[unlikely]] {
        LOG_INFO(Loader, "Not a SELF file. Magic mismatch current = {:#x} expected = {:#x}",
                 m_self.magic, self_header::signature);
        return false;
    }

    if (m_self.version != 0x00 || m_self.mode != 0x01 || m_self.endian != 0x01 ||
        m_self.attributes != 0x12) [[unlikely]] {
        LOG_INFO(Loader, "Unknown SELF file");
        return false;
    }

    if (m_self.category != 0x01 || m_self.program_type != 0x01) [[unlikely]] {
        LOG_INFO(Loader, "Unknown SELF file");
        return false;
    }

    return true;
}

bool Elf::IsElfFile() const {
    if (m_elf_header.e_ident.magic[EI_MAG0] != ELFMAG0 ||
        m_elf_header.e_ident.magic[EI_MAG1] != ELFMAG1 ||
        m_elf_header.e_ident.magic[EI_MAG2] != ELFMAG2 ||
        m_elf_header.e_ident.magic[EI_MAG3] != ELFMAG3) {
        LOG_INFO(Loader, "Not an ELF file magic is wrong!");
        return false;
    }
    if (m_elf_header.e_ident.ei_class != ELF_CLASS_64) {
        LOG_INFO(Loader, "e_ident[EI_CLASS] expected 0x02 is ({:#x})",
                 static_cast<u32>(m_elf_header.e_ident.ei_class));
        return false;
    }

    if (m_elf_header.e_ident.ei_data != ELF_DATA_2LSB) {
        LOG_INFO(Loader, "e_ident[EI_DATA] expected 0x01 is ({:#x})",
                 static_cast<u32>(m_elf_header.e_ident.ei_data));
        return false;
    }

    if (m_elf_header.e_ident.ei_version != ELF_VERSION_CURRENT) {
        LOG_INFO(Loader, "e_ident[EI_VERSION] expected 0x01 is ({:#x})",
                 static_cast<u32>(m_elf_header.e_ident.ei_version));
        return false;
    }

    if (m_elf_header.e_ident.ei_osabi != ELF_OSABI_FREEBSD) {
        LOG_INFO(Loader, "e_ident[EI_OSABI] expected 0x09 is ({:#x})",
                 static_cast<u32>(m_elf_header.e_ident.ei_osabi));
        return false;
    }

    if (m_elf_header.e_ident.ei_abiversion != ELF_ABI_VERSION_AMDGPU_HSA_V2) {
        LOG_INFO(Loader, "e_ident[EI_ABIVERSION] expected 0x00 is ({:#x})",
                 static_cast<u32>(m_elf_header.e_ident.ei_abiversion));
        return false;
    }

    if (m_elf_header.e_type != ET_SCE_DYNEXEC && m_elf_header.e_type != ET_SCE_DYNAMIC &&
        m_elf_header.e_type != ET_SCE_EXEC) {
        LOG_INFO(Loader, "e_type expected 0xFE10 OR 0xFE18 OR 0xfe00 is ({:#x})",
                 static_cast<u32>(m_elf_header.e_type));
        return false;
    }

    if (m_elf_header.e_machine != EM_X86_64) {
        LOG_INFO(Loader, "e_machine expected 0x3E is ({:#x})",
                 static_cast<u32>(m_elf_header.e_machine));
        return false;
    }

    if (m_elf_header.e_version != EV_CURRENT) {
        LOG_INFO(Loader, "m_elf_header.e_version expected 0x01 is ({:#x})",
                 static_cast<u32>(m_elf_header.e_version));
        return false;
    }

    if (m_elf_header.e_phentsize != sizeof(elf_program_header)) {
        LOG_INFO(Loader, "e_phentsize ({}) != sizeof(elf_program_header)",
                 m_elf_header.e_phentsize);
        return false;
    }

    if (m_elf_header.e_shentsize > 0 &&
        m_elf_header.e_shentsize !=
            sizeof(elf_section_header)) // Commercial games doesn't appear to have section headers
    {
        LOG_INFO(Loader, "e_shentsize ({}) != sizeof(elf_section_header)",
                 m_elf_header.e_shentsize);
        return false;
    }

    return true;
}

std::string Elf::SElfHeaderStr() {
    std::string header = fmt::format("======= SELF HEADER =========\n", m_self.magic);
    header += fmt::format("magic ..............: 0x{:X}\n", m_self.magic);
    header += fmt::format("version ............: {}\n", m_self.version);
    header += fmt::format("mode ...............: {:#04x}\n", m_self.mode);
    header += fmt::format("endian .............: {}\n", m_self.endian);
    header += fmt::format("attributes .........: {:#04x}\n", m_self.attributes);
    header += fmt::format("category ...........: {:#04x}\n", m_self.category);
    header += fmt::format("program_type........: {:#04x}\n", m_self.program_type);
    header += fmt::format("padding1 ...........: {:#06x}\n", m_self.padding1);
    header += fmt::format("header size ........: {}\n", m_self.header_size);
    header += fmt::format("meta size ..........: {}\n", m_self.meta_size);
    header += fmt::format("file size ..........: {}\n", m_self.file_size);
    header += fmt::format("padding2 ...........: {:#010x}\n", m_self.padding2);
    header += fmt::format("segment count ......: {}\n", m_self.segment_count);
    header += fmt::format("unknown 1A .........: {:#06x}\n", m_self.unknown1A);
    header += fmt::format("padding3 ...........: {:#010x}\n", m_self.padding3);
    return header;
}

std::string Elf::SELFSegHeader(u16 no) {
    const auto segment_header = m_self_segments[no];
    std::string header = fmt::format("====== SEGMENT HEADER {} ========\n", no);
    header += fmt::format("flags ............: {:#018x}\n", segment_header.flags);
    header += fmt::format("file offset ......: {:#018x}\n", segment_header.file_offset);
    header += fmt::format("file size ........: {}\n", segment_header.file_size);
    header += fmt::format("memory size ......: {}\n", segment_header.memory_size);
    return header;
}

std::string Elf::ElfHeaderStr() {
    std::string header = fmt::format("======= Elf header ===========\n");
    header += fmt::format("ident ............: 0x");
    for (auto i : m_elf_header.e_ident.magic) {
        header += fmt::format("{:02X}", i);
    }
    header += fmt::format("\n");

    header +=
        fmt::format("ident class.......: {}\n", GetIdentClassName(m_elf_header.e_ident.ei_class));
    header +=
        fmt::format("ident data .......: {}\n", GetIdentEndianName(m_elf_header.e_ident.ei_data));
    header += fmt::format("ident version.....: {}\n",
                          GetIdentVersionName(m_elf_header.e_ident.ei_version));
    header +=
        fmt::format("ident osabi  .....: {}\n", GetIdentOsabiName(m_elf_header.e_ident.ei_osabi));
    header += fmt::format("ident abiversion..: {}\n",
                          GetIdentAbiversionName(m_elf_header.e_ident.ei_abiversion));

    header += fmt::format("ident UNK ........: 0x");
    for (auto i : m_elf_header.e_ident.pad) {
        header += fmt::format("{:02X}", i);
    }
    header += fmt::format("\n");

    header += fmt::format("type  ............: {}\n", GetType(m_elf_header.e_type));
    header += fmt::format("machine ..........: {}\n", GetMachine(m_elf_header.e_machine));
    header += fmt::format("version ..........: {}\n", GetVersion(m_elf_header.e_version));
    header += fmt::format("entry ............: {:#018x}\n", m_elf_header.e_entry);
    header += fmt::format("phoff ............: {:#018x}\n", m_elf_header.e_phoff);
    header += fmt::format("shoff ............: {:#018x}\n", m_elf_header.e_shoff);
    header += fmt::format("flags ............: {:#010x}\n", m_elf_header.e_flags);
    header += fmt::format("ehsize ...........: {}\n", m_elf_header.e_ehsize);
    header += fmt::format("phentsize ........: {}\n", m_elf_header.e_phentsize);
    header += fmt::format("phnum ............: {}\n", m_elf_header.e_phnum);
    header += fmt::format("shentsize ........: {}\n", m_elf_header.e_shentsize);
    header += fmt::format("shnum ............: {}\n", m_elf_header.e_shnum);
    header += fmt::format("shstrndx .........: {}\n", m_elf_header.e_shstrndx);
    return header;
}

std::string_view Elf::ElfPheaderTypeStr(u32 type) {
    switch (type) {
    case PT_NULL:
        return "Null";
    case PT_LOAD:
        return "Loadable";
    case PT_DYNAMIC:
        return "Dynamic";
    case PT_INTERP:
        return "Interpreter Path";
    case PT_NOTE:
        return "Note";
    case PT_SHLIB:
        return "Section Header Library";
    case PT_PHDR:
        return "Program Header";
    case PT_TLS:
        return "Thread-Local Storage";
    case PT_NUM:
        return "Defined Sections Number";
    case PT_SCE_RELA:
        return "SCE Relative";
    case PT_SCE_DYNLIBDATA:
        return "SCE Dynamic Library Data";
    case PT_SCE_PROCPARAM:
        return "SCE Processor Parameters";
    case PT_SCE_MODULE_PARAM:
        return "SCE Module Parameters";
    case PT_SCE_RELRO:
        return "SCE Read-Only After Relocation";
    case PT_GNU_EH_FRAME:
        return "GNU Entry Header Frame";
    case PT_GNU_STACK:
        return "GNU Stack (executability)";
    case PT_GNU_RELRO:
        return "GNU Read-Only After Relocation";
    case PT_SCE_COMMENT:
        return "SCE Comment";
    case PT_SCE_LIBVERSION:
        return "SCE Library Version";
    default:
        return "Unknown Section";
    }
}

std::string Elf::ElfPheaderFlagsStr(u32 flags) {
    std::string flg = "(";
    flg += (flags & PF_READ) ? "R" : "_";
    flg += (flags & PF_WRITE) ? "W" : "_";
    flg += (flags & PF_EXEC) ? "X" : "_";
    flg += ")";
    return flg;
}

std::string Elf::ElfPHeaderStr(u16 no) {
    std::string header = fmt::format("====== PROGRAM HEADER {} ========\n", no);
    header += fmt::format("p_type ....: {}\n", ElfPheaderTypeStr(m_elf_phdr[no].p_type));
    header += fmt::format("p_flags ...: {:#010x}\n", static_cast<u32>(m_elf_phdr[no].p_flags));
    header += fmt::format("p_offset ..: {:#018x}\n", m_elf_phdr[no].p_offset);
    header += fmt::format("p_vaddr ...: {:#018x}\n", m_elf_phdr[no].p_vaddr);
    header += fmt::format("p_paddr ...: {:#018x}\n", m_elf_phdr[no].p_paddr);
    header += fmt::format("p_filesz ..: {:#018x}\n", m_elf_phdr[no].p_filesz);
    header += fmt::format("p_memsz ...: {:#018x}\n", m_elf_phdr[no].p_memsz);
    header += fmt::format("p_align ...: {:#018x}\n", m_elf_phdr[no].p_align);
    return header;
}

void Elf::LoadSegment(u64 virtual_addr, u64 file_offset, u64 size) {
    if (!is_self) {
        // It's elf file
        if (!m_f.Seek(file_offset, SeekOrigin::SetOrigin)) {
            LOG_CRITICAL(Loader, "Failed to seek to ELF header");
            return;
        }
        m_f.ReadRaw<u8>(reinterpret_cast<u8*>(virtual_addr), size);
        return;
    }

    for (uint16_t i = 0; i < m_self.segment_count; i++) {
        const auto& seg = m_self_segments[i];

        if (seg.IsBlocked()) {
            auto phdr_id = seg.GetId();
            const auto& phdr = m_elf_phdr[phdr_id];

            if (file_offset >= phdr.p_offset && file_offset < phdr.p_offset + phdr.p_filesz) {
                auto offset = file_offset - phdr.p_offset;
                if (!m_f.Seek(offset + seg.file_offset, SeekOrigin::SetOrigin)) {
                    LOG_CRITICAL(Loader, "Failed to seek to segment");
                    return;
                }
                m_f.ReadRaw<u8>(reinterpret_cast<u8*>(virtual_addr), size);
                return;
            }
        }
    }
    UNREACHABLE();
}

bool Elf::IsSharedLib() {
    return m_elf_header.e_type == ET_SCE_DYNAMIC;
}

void Elf::ElfHeaderDebugDump(const std::filesystem::path& file_name) {
    Common::FS::IOFile f{file_name, Common::FS::FileAccessMode::Create,
                         Common::FS::FileType::TextFile};
    f.WriteString(ElfHeaderStr());
}

void Elf::SelfHeaderDebugDump(const std::filesystem::path& file_name) {
    Common::FS::IOFile f{file_name, Common::FS::FileAccessMode::Create,
                         Common::FS::FileType::TextFile};
    f.WriteString(SElfHeaderStr());
}

void Elf::SelfSegHeaderDebugDump(const std::filesystem::path& file_name) {
    Common::FS::IOFile f{file_name, Common::FS::FileAccessMode::Create,
                         Common::FS::FileType::TextFile};
    for (u16 i = 0; i < m_self.segment_count; i++) {
        f.WriteString(SELFSegHeader(i));
    }
}

void Elf::PHeaderDebugDump(const std::filesystem::path& file_name) {
    Common::FS::IOFile f{file_name, Common::FS::FileAccessMode::Create,
                         Common::FS::FileType::TextFile};
    if (m_elf_header.e_phentsize > 0) {
        for (u16 i = 0; i < m_elf_header.e_phnum; i++) {
            f.WriteString(ElfPHeaderStr(i));
        }
    }
}

} // namespace Core::Loader
