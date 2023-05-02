#include "Elf.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/pattern_formatter.h>
#include <magic_enum.hpp>

template <>
struct magic_enum::customize::enum_range<e_type_s> {
    static constexpr int min = 0xfe00;
    static constexpr int max = 0xfe18;
    // (max - min) must be less than UINT16_MAX.
};

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
    delete m_self_id_header;
    delete[] m_self_segments;
    delete[] m_elf_phdr;
    delete[] m_elf_shdr;

    m_self = nullptr;
    m_self_segments = nullptr;
    m_elf_header = nullptr;
    m_elf_phdr = nullptr;
    m_elf_shdr = nullptr;
    m_self_id_header = nullptr;
}
void Elf::Open(const std::string& file_name)
{
    Reset();//reset all variables

	m_f = new FsFile;
	m_f->Open(file_name, fsOpenMode::fsRead);

	m_self = load_self(*m_f);

    bool isself = isSelfFile();
    if (!isself)
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
    if (isself && m_elf_header != nullptr)
    {
        u64 header_size = 0;
        header_size += sizeof(self_header);
        header_size += sizeof(self_segment_header) * m_self->segment_count;
        header_size += sizeof(elf_header);
        header_size += m_elf_header->e_phnum * m_elf_header->e_phentsize;
        header_size += m_elf_header->e_shnum * m_elf_header->e_shentsize;
        header_size += 15; header_size &= ~15; // align

        if (m_elf_header->e_ehsize - header_size >= sizeof(elf_program_id_header))
        {
            m_f->Seek(header_size, fsSeekMode::fsSeekSet);

            m_self_id_header = new elf_program_id_header;
            m_f->Read(m_self_id_header, sizeof(elf_program_id_header));

        }
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

    if (m_elf_header->e_ident.magic[EI_MAG0] != ELFMAG0 || m_elf_header->e_ident.magic[EI_MAG1] != ELFMAG1 || m_elf_header->e_ident.magic[EI_MAG2] != ELFMAG2 ||
        m_elf_header->e_ident.magic[EI_MAG3] != ELFMAG3)
    {
        printf("ERROR:Not an ELF file magic is wrong!\n");
        return false;
    }
    if (m_elf_header->e_ident.ei_class != ELF_CLASS_64)
    {
        printf("ERROR:e_ident[EI_CLASS] expected 0x02 is (0x%x)\n", m_elf_header->e_ident.ei_class);
        return false;
    }

    if (m_elf_header->e_ident.ei_data != ELF_DATA_2LSB)
    {
        printf("ERROR:e_ident[EI_DATA] expected 0x01 is (0x%x)\n", m_elf_header->e_ident.ei_data);
        return false;
    }

    if (m_elf_header->e_ident.ei_version != ELF_VERSION_CURRENT)
    {
        printf("ERROR:e_ident[EI_VERSION] expected 0x01 is (0x%x)\n", m_elf_header->e_ident.ei_version);
        return false;
    }

    if (m_elf_header->e_ident.ei_osabi != ELF_OSABI_FREEBSD)
    {
        printf("ERROR:e_ident[EI_OSABI] expected 0x09 is (0x%x)\n", m_elf_header->e_ident.ei_osabi);
        return false;
    }

    if (m_elf_header->e_ident.ei_abiversion != ELF_ABI_VERSION_AMDGPU_HSA_V2)
    {
        printf("ERROR:e_ident[EI_ABIVERSION] expected 0x00 is (0x%x)\n", m_elf_header->e_ident.ei_abiversion);
        return false;
    }

    if (m_elf_header->e_type != ET_SCE_DYNEXEC && m_elf_header->e_type != ET_SCE_DYNAMIC)
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
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(L"output.log", true)); //this might work only in windows ;/
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("shadps4 logger", begin(sinks), end(sinks)));   
    auto f = std::make_unique<spdlog::pattern_formatter>("%v", spdlog::pattern_time_type::local, std::string(""));  // disable eol
    spdlog::set_formatter(std::move(f));
    spdlog::info("SELF header:\n");
    
    spdlog::info("  magic ..............: 0x{:X}\n", m_self->magic);
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
    spdlog::info(" ident ............: 0x");
    for (auto i : m_elf_header->e_ident.magic)
    {
        spdlog::info("{:02X}", i);
    }
    spdlog::info("\n");

    auto ident_class = magic_enum::enum_cast<ident_class_es>(m_elf_header->e_ident.ei_class);
    if (ident_class.has_value())
    {
        spdlog::info(" ident class.......: {}\n", magic_enum::enum_name(ident_class.value()));
    }

    auto ident_data = magic_enum::enum_cast<ident_endian_es>(m_elf_header->e_ident.ei_data);
    if (ident_data.has_value())
    {
        spdlog::info(" ident data .......: {}\n", magic_enum::enum_name(ident_data.value()));
    }

    auto ident_version = magic_enum::enum_cast<ident_version_es>(m_elf_header->e_ident.ei_version);
    if (ident_version.has_value())
    {
        spdlog::info(" ident version.....: {}\n", magic_enum::enum_name(ident_version.value()));
    }

    auto ident_osabi = magic_enum::enum_cast<ident_osabi_es>(m_elf_header->e_ident.ei_osabi);
    if (ident_osabi.has_value())
    {
        spdlog::info(" ident osabi  .....: {}\n", magic_enum::enum_name(ident_osabi.value()));
    }

    auto ident_abiversion = magic_enum::enum_cast<ident_abiversion_es>(m_elf_header->e_ident.ei_abiversion);
    if (ident_abiversion.has_value())
    {
        spdlog::info(" ident abiversion..: {}\n", magic_enum::enum_name(ident_abiversion.value()));
    }

    spdlog::info(" ident UNK ........: 0x");
    for (auto i : m_elf_header->e_ident.pad)
    {
        spdlog::info("{:02X}", i);
    }
    spdlog::info("\n");

    auto type = magic_enum::enum_cast<e_type_s>(m_elf_header->e_type);
    if (type.has_value())
    {
        spdlog::info(" type  ............: {}\n", magic_enum::enum_name(type.value()));
    }
    
    auto machine = magic_enum::enum_cast<e_machine_es>(m_elf_header->e_machine);
    if (machine.has_value())
    {
        spdlog::info(" machine ..........: {}\n", magic_enum::enum_name(machine.value()));
    }
    auto version = magic_enum::enum_cast<e_version_es>(m_elf_header->e_version);
    if (version.has_value())
    {
        spdlog::info(" version ..........: {}\n", magic_enum::enum_name(version.value()));
    }
    spdlog::info(" entry ............: {:#018x}\n", m_elf_header->e_entry);
    spdlog::info(" phoff ............: {:#018x}\n", m_elf_header->e_phoff);
    spdlog::info(" shoff ............: {:#018x}\n", m_elf_header->e_shoff);
    spdlog::info(" flags ............: {:#010x}\n", m_elf_header->e_flags);
    spdlog::info(" ehsize ...........: {}\n", m_elf_header->e_ehsize);
    spdlog::info(" phentsize ........: {}\n", m_elf_header->e_phentsize);
    spdlog::info(" phnum ............: {}\n", m_elf_header->e_phnum);
    spdlog::info(" shentsize ........: {}\n", m_elf_header->e_shentsize);
    spdlog::info(" shnum ............: {}\n", m_elf_header->e_shnum);
    spdlog::info(" shstrndx .........: {}\n", m_elf_header->e_shstrndx);

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
    if (m_self_id_header != nullptr)
    {
        spdlog::info("SELF info:\n");
        spdlog::info("auth id ............: {:#018x}\n", m_self_id_header->authid);
        auto program_type = magic_enum::enum_cast<program_type_es>(m_self_id_header->program_type);
        if (program_type.has_value())
        {
            spdlog::info("program type .......: {}\n", magic_enum::enum_name(program_type.value()));
        }
        else
        {
            spdlog::info("program type UNK....: {:#018x}\n", (int)m_self_id_header->program_type);
        }
        spdlog::info("app version ........: {:#018x}\n", m_self_id_header->appver);
        spdlog::info("fw version .........: {:#018x}\n", m_self_id_header->firmver);
        spdlog::info("digest..............: 0x");
        for (int i = 0; i < 32; i++)  spdlog::info("{:02x}", m_self_id_header->digest[i]);
        spdlog::info("\n");
    }
}