#include "Elf.h"
#include <fmt/core.h>

Elf::~Elf()
{
    Reset();
}

static self_header* load_self(FsFile& f)
{
	//read self header
	auto* self = new self_header;
	f.Read(self, sizeof(self_header));
	return self;
}


static self_segment_header* load_self_segments(FsFile& f, u16 num)
{
    auto* segs = new self_segment_header[num];

    f.Read(segs, sizeof(self_segment_header) * num);

    return segs;
}


static elf_header* load_elf_header(FsFile& f)
{
    auto* m_elf_header = new elf_header;

    f.Read(m_elf_header, sizeof(elf_header));

    return m_elf_header;
}

static elf_program_header* load_program_header(FsFile& f, u64 offset, u16 num)
{
    auto* phdr = new elf_program_header[num];

    f.Seek(offset,fsSeekMode::fsSeekSet);
    f.Read(phdr, sizeof(elf_program_header) * num);

    return phdr;
}

static elf_section_header* load_section_header(FsFile& f, u64 offset, u16 num)
{
    if (num == 0)//just in case we don't have section headers
    {
        return nullptr;
    }

    auto* shdr = new elf_section_header[num];

    f.Seek(offset,fsSeekMode::fsSeekSet);
    f.Read(shdr, sizeof(elf_section_header) * num);

    return shdr;
}

void Elf::Reset()//reset all variables
{
    if (m_f != nullptr)
    {
        m_f->Close();
        delete m_f;
    }
    delete m_self;
    delete m_elf_header;
    delete[] m_self_segments;
    delete[] m_elf_phdr;
    delete[] m_elf_shdr;

    m_self = nullptr;
    m_self_segments = nullptr;
    m_elf_header = nullptr;
    m_elf_phdr = nullptr;
    m_elf_shdr = nullptr;
}
void Elf::Open(const std::string& file_name)
{
    Reset();//reset all variables

	m_f = new FsFile;
	m_f->Open(file_name, fsOpenMode::fsRead);

	m_self = load_self(*m_f);

    if (!isSelfFile())
    {
        delete m_self;
        m_self = nullptr;
        m_f->Seek(0,fsSeekMode::fsSeekSet); //it is not an self file move to the start of file
    }
    else
    {
        m_self_segments = load_self_segments(*m_f, m_self->segment_count);
    }

    auto elfheader_pos = m_f->Tell();//get the entry pos for elf file

    m_elf_header = load_elf_header(*m_f);

    if (!isElfFile())
    {
        delete m_elf_header;
        m_elf_header = nullptr;
    }

    if (m_elf_header != nullptr)
    {
        m_elf_phdr = load_program_header(*m_f, elfheader_pos + m_elf_header->e_phoff, m_elf_header->e_phnum);
        m_elf_shdr = load_section_header(*m_f, elfheader_pos + m_elf_header->e_shoff, m_elf_header->e_shnum);
    }
    DebugDump();
}

bool Elf::isSelfFile() const
{
    if (m_f == nullptr)
    {
        return false;
    }

    if (m_self == nullptr)
    {
        return false;//if we can't load self header return false
    }

    if (m_self->magic != self_header::signature)
    {
        printf("Not a SELF file. Magic file mismatched! current = 0x%08X required = 0x%08X\n", m_self->magic, self_header::signature);
        return false;
    }

    if (m_self->version != 0x00 || m_self->mode != 0x01 || m_self->endian != 0x01 || m_self->attributes != 0x12)
    {
        printf("Unknown SELF file\n");
        return false;
    }

    if (m_self->category != 0x01 || m_self->program_type != 0x01)
    {
        printf("Unknown SELF file\n");
        return false;
    }

    return true;
}

bool Elf::isElfFile() const
{
    if (m_f == nullptr)
    {
        return false;
    }

    if (m_elf_header == nullptr)
    {
        return false;
    }
    if (m_elf_header->e_ident[EI_MAG0] != ELFMAG0 || m_elf_header->e_ident[EI_MAG1] != ELFMAG1 || m_elf_header->e_ident[EI_MAG2] != ELFMAG2 ||
        m_elf_header->e_ident[EI_MAG3] != ELFMAG3)
    {
        printf("ERROR:Not an ELF file magic is wrong!\n");
        return false;
    }
    if (m_elf_header->e_ident[EI_CLASS] != ELFCLASS64)
    {
        printf("ERROR:e_ident[EI_CLASS] expected 0x02 is (0x%x)\n", m_elf_header->e_ident[EI_CLASS]);
        return false;
    }

    if (m_elf_header->e_ident[EI_DATA] != ELFDATA2LSB)
    {
        printf("ERROR:e_ident[EI_DATA] expected 0x01 is (0x%x)\n", m_elf_header->e_ident[EI_DATA]);
        return false;
    }

    if (m_elf_header->e_ident[EI_VERSION] != EV_CURRENT)
    {
        printf("ERROR:e_ident[EI_VERSION] expected 0x01 is (0x%x)\n", m_elf_header->e_ident[EI_VERSION]);
        return false;
    }

    if (m_elf_header->e_ident[EI_OSABI] != ELFOSABI_FREEBSD)
    {
        printf("ERROR:e_ident[EI_OSABI] expected 0x09 is (0x%x)\n", m_elf_header->e_ident[EI_OSABI]);
        return false;
    }

    if (m_elf_header->e_ident[EI_ABIVERSION] != ELFABIVERSION_AMDGPU_HSA_V2)
    {
        printf("ERROR:e_ident[EI_ABIVERSION] expected 0x00 is (0x%x)\n", m_elf_header->e_ident[EI_ABIVERSION]);
        return false;
    }

    if (m_elf_header->e_type != ET_DYNEXEC && m_elf_header->e_type != ET_DYNAMIC)
    {
        printf("ERROR:e_type expected 0xFE10 OR 0xFE18 is (%04x)\n", m_elf_header->e_type);
        return false;
    }

    if (m_elf_header->e_machine != EM_X86_64)
    {
        printf("ERROR:e_machine expected 0x3E is (%04x)\n", m_elf_header->e_machine);
        return false;
    }

    if (m_elf_header->e_version != EV_CURRENT)
    {
        printf("ERROR:m_elf_header->e_version expected 0x01 is (0x%x)\n", m_elf_header->e_version);
        return false;
    }

    if (m_elf_header->e_phentsize != sizeof(elf_program_header))
    {
        printf("ERROR:e_phentsize (%d) != sizeof(elf_program_header)\n", m_elf_header->e_phentsize);
        return false;
    }

    if (m_elf_header->e_shentsize > 0 && m_elf_header->e_shentsize != sizeof(elf_section_header)) //commercial games doesn't appear to have section headers
    {
        printf("ERROR:e_shentsize (%d) != sizeof(elf_section_header)\n", m_elf_header->e_shentsize);
        return false;
    }

    return true;
}

void Elf::DebugDump() {
    printf("SELF header:\n");
    fmt::print("  magic ..............: 0x{:x}\n", m_self->magic);
    fmt::print("  version    .........: {}\n", m_self->version);
    fmt::print("  mode       .........: {:#04x}\n", m_self->mode);
    fmt::print("  endian     .........: {}\n", m_self->endian);
    fmt::print("  attributes .........: {:#04x}\n", m_self->attributes);
    fmt::print("  category   .........: {:#04x}\n", m_self->category);
    fmt::print("  program_type........: {:#04x}\n", m_self->program_type);
    fmt::print("  padding1 ...........: {:#06x}\n", m_self->padding1);
    fmt::print("  header size ........: {}\n", m_self->header_size);
    fmt::print("  meta size      .....: {}\n", m_self->meta_size);
    fmt::print("  file size ..........: {}\n", m_self->file_size);
    fmt::print("  padding2 ...........: {:#010x}\n", m_self->padding2);
    fmt::print("  segment count ......: {}\n", m_self->segment_count);
    fmt::print("  unknown 1A .........: {:#06x}\n", m_self->unknown1A);
    fmt::print("  padding3 ...........: {:#010x}\n", m_self->padding3);
    fmt::print("\n");

    fmt::print("SELF segments:\n");

    for (int i = 0; i < m_self->segment_count; i++)
    {
        auto segment_header = m_self_segments[i];
        fmt::print(" [{}]\n", i);
        fmt::print("  flags ............: {:#018x}\n", segment_header.flags);
        fmt::print("  file offset ......: {:#018x}\n", segment_header.file_offset);
        fmt::print("  file size ........: {}\n", segment_header.file_size);
        fmt::print("  memory size ......: {}\n", segment_header.memory_size);
    }
    fmt::print("\n");

    fmt::print("Elf header:\n");
    fmt::print(" ident .........: 0x");
    for (auto i : m_elf_header->e_ident)
    {
        fmt::print("{:02x}", i);
    }
    fmt::print("\n");

    fmt::print(" type  .........: {:#06x}\n", m_elf_header->e_type);
    fmt::print(" machine .......: {:#06x}\n", m_elf_header->e_machine);
    fmt::print(" version .......: {:#010x}\n", m_elf_header->e_version);

    fmt::print(" entry .........: {:#018x}\n", m_elf_header->e_entry);
    fmt::print(" phoff .........: {:#018x}\n", m_elf_header->e_phoff);
    fmt::print(" shoff .........: {:#018x}\n", m_elf_header->e_shoff);
    fmt::print(" flags .........: {:#010x}\n", m_elf_header->e_flags);
    fmt::print(" ehsize ........: {}\n", m_elf_header->e_ehsize);
    fmt::print(" phentsize .....: {}\n", m_elf_header->e_phentsize);
    fmt::print(" phnum .........: {}\n", m_elf_header->e_phnum);
    fmt::print(" shentsize .....: {}\n", m_elf_header->e_shentsize);
    fmt::print(" shnum .........: {}\n", m_elf_header->e_shnum);
    fmt::print(" shstrndx ......: {}\n", m_elf_header->e_shstrndx);

    if (m_elf_header->e_phentsize > 0)
    {
        fmt::print("Program headers:\n");
        for (u16 i = 0; i < m_elf_header->e_phnum; i++)
        {
            fmt::print("--- phdr [{}] ---\n", i);
            fmt::print("p_type ....: {:#010x}\n", (m_elf_phdr+i)->p_type);
            fmt::print("p_flags ...: {:#010x}\n", (m_elf_phdr + i)->p_flags);
            fmt::print("p_offset ..: {:#018x}\n", (m_elf_phdr + i)->p_offset);
            fmt::print("p_vaddr ...: {:#018x}\n", (m_elf_phdr + i)->p_vaddr);
            fmt::print("p_paddr ...: {:#018x}\n", (m_elf_phdr + i)->p_paddr);
            fmt::print("p_filesz ..: {:#018x}\n", (m_elf_phdr + i)->p_filesz);
            fmt::print("p_memsz ...: {:#018x}\n", (m_elf_phdr + i)->p_memsz);
            fmt::print("p_align ...: {:#018x}\n", (m_elf_phdr + i)->p_align);
        }
    }
    if (m_elf_header->e_shentsize > 0)
    {
        fmt::print("Section headers:\n");
        for (u16 i = 0; i < m_elf_header->e_shnum; i++)
        {
            fmt::print("--- shdr {} --\n", i);
            fmt::print("sh_name ........: {}\n", (m_elf_shdr + i)->sh_name);
            fmt::print("sh_type ........: {:#010x}\n", (m_elf_shdr + i)->sh_type);
            fmt::print("sh_flags .......: {:#018x}\n", (m_elf_shdr + i)->sh_flags);
            fmt::print("sh_addr ........: {:#018x}\n", (m_elf_shdr + i)->sh_addr);
            fmt::print("sh_offset ......: {:#018x}\n", (m_elf_shdr + i)->sh_offset);
            fmt::print("sh_size ........: {:#018x}\n", (m_elf_shdr + i)->sh_size);
            fmt::print("sh_link ........: {:#010x}\n", (m_elf_shdr + i)->sh_link);
            fmt::print("sh_info ........: {:#010x}\n", (m_elf_shdr + i)->sh_info);
            fmt::print("sh_addralign ...: {:#018x}\n", (m_elf_shdr + i)->sh_addralign);
            fmt::print("sh_entsize .....: {:#018x}\n", (m_elf_shdr + i)->sh_entsize);
        }
    }
}