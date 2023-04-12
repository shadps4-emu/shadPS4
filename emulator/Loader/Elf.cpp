#include "Elf.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <spdlog/sinks/stdout_color_sinks.h>

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
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("output.log", true));
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("shadps4 logger", begin(sinks), end(sinks)));   
    auto f = std::make_unique<spdlog::pattern_formatter>("%v", spdlog::pattern_time_type::local, std::string(""));  // disable eol
    spdlog::set_formatter(std::move(f));
    spdlog::info("SELF header:\n");
    
    spdlog::info("  magic ..............: 0x{:x}\n", m_self->magic);
    spdlog::info("  version    .........: {}\n", m_self->version);
    spdlog::info("  mode       .........: {:#04x}\n", m_self->mode);
    spdlog::info("  endian     .........: {}\n", m_self->endian);
    spdlog::info("  attributes .........: {:#04x}\n", m_self->attributes);
    spdlog::info("  category   .........: {:#04x}\n", m_self->category);
    spdlog::info("  program_type........: {:#04x}\n", m_self->program_type);
    spdlog::info("  padding1 ...........: {:#06x}\n", m_self->padding1);
    spdlog::info("  header size ........: {}\n", m_self->header_size);
    spdlog::info("  meta size      .....: {}\n", m_self->meta_size);
    spdlog::info("  file size ..........: {}\n", m_self->file_size);
    spdlog::info("  padding2 ...........: {:#010x}\n", m_self->padding2);
    spdlog::info("  segment count ......: {}\n", m_self->segment_count);
    spdlog::info("  unknown 1A .........: {:#06x}\n", m_self->unknown1A);
    spdlog::info("  padding3 ...........: {:#010x}\n", m_self->padding3);
    spdlog::info("\n");

    spdlog::info("SELF segments:\n");

    for (int i = 0; i < m_self->segment_count; i++)
    {
        auto segment_header = m_self_segments[i];
        spdlog::info(" [{}]\n", i);
        spdlog::info("  flags ............: {:#018x}\n", segment_header.flags);
        spdlog::info("  file offset ......: {:#018x}\n", segment_header.file_offset);
        spdlog::info("  file size ........: {}\n", segment_header.file_size);
        spdlog::info("  memory size ......: {}\n", segment_header.memory_size);
    }
    spdlog::info("\n");

    spdlog::info("Elf header:\n");
    spdlog::info(" ident .........: 0x");
    for (auto i : m_elf_header->e_ident)
    {
        spdlog::info("{:02x}", i);
    }
    spdlog::info("\n");

    spdlog::info(" type  .........: {:#06x}\n", m_elf_header->e_type);
    spdlog::info(" machine .......: {:#06x}\n", m_elf_header->e_machine);
    spdlog::info(" version .......: {:#010x}\n", m_elf_header->e_version);

    spdlog::info(" entry .........: {:#018x}\n", m_elf_header->e_entry);
    spdlog::info(" phoff .........: {:#018x}\n", m_elf_header->e_phoff);
    spdlog::info(" shoff .........: {:#018x}\n", m_elf_header->e_shoff);
    spdlog::info(" flags .........: {:#010x}\n", m_elf_header->e_flags);
    spdlog::info(" ehsize ........: {}\n", m_elf_header->e_ehsize);
    spdlog::info(" phentsize .....: {}\n", m_elf_header->e_phentsize);
    spdlog::info(" phnum .........: {}\n", m_elf_header->e_phnum);
    spdlog::info(" shentsize .....: {}\n", m_elf_header->e_shentsize);
    spdlog::info(" shnum .........: {}\n", m_elf_header->e_shnum);
    spdlog::info(" shstrndx ......: {}\n", m_elf_header->e_shstrndx);

    if (m_elf_header->e_phentsize > 0)
    {
        spdlog::info("Program headers:\n");
        for (u16 i = 0; i < m_elf_header->e_phnum; i++)
        {
            spdlog::info("--- phdr [{}] ---\n", i);
            spdlog::info("p_type ....: {:#010x}\n", (m_elf_phdr+i)->p_type);
            spdlog::info("p_flags ...: {:#010x}\n", (m_elf_phdr + i)->p_flags);
            spdlog::info("p_offset ..: {:#018x}\n", (m_elf_phdr + i)->p_offset);
            spdlog::info("p_vaddr ...: {:#018x}\n", (m_elf_phdr + i)->p_vaddr);
            spdlog::info("p_paddr ...: {:#018x}\n", (m_elf_phdr + i)->p_paddr);
            spdlog::info("p_filesz ..: {:#018x}\n", (m_elf_phdr + i)->p_filesz);
            spdlog::info("p_memsz ...: {:#018x}\n", (m_elf_phdr + i)->p_memsz);
            spdlog::info("p_align ...: {:#018x}\n", (m_elf_phdr + i)->p_align);
        }
    }
    if (m_elf_header->e_shentsize > 0)
    {
        spdlog::info("Section headers:\n");
        for (u16 i = 0; i < m_elf_header->e_shnum; i++)
        {
            spdlog::info("--- shdr {} --\n", i);
            spdlog::info("sh_name ........: {}\n", (m_elf_shdr + i)->sh_name);
            spdlog::info("sh_type ........: {:#010x}\n", (m_elf_shdr + i)->sh_type);
            spdlog::info("sh_flags .......: {:#018x}\n", (m_elf_shdr + i)->sh_flags);
            spdlog::info("sh_addr ........: {:#018x}\n", (m_elf_shdr + i)->sh_addr);
            spdlog::info("sh_offset ......: {:#018x}\n", (m_elf_shdr + i)->sh_offset);
            spdlog::info("sh_size ........: {:#018x}\n", (m_elf_shdr + i)->sh_size);
            spdlog::info("sh_link ........: {:#010x}\n", (m_elf_shdr + i)->sh_link);
            spdlog::info("sh_info ........: {:#010x}\n", (m_elf_shdr + i)->sh_info);
            spdlog::info("sh_addralign ...: {:#018x}\n", (m_elf_shdr + i)->sh_addralign);
            spdlog::info("sh_entsize .....: {:#018x}\n", (m_elf_shdr + i)->sh_entsize);
        }
    }
}