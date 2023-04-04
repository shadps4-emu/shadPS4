#include "Elf.h"


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

    m_self = nullptr;
    m_self_segments = nullptr;
    m_elf_header = nullptr;
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
    if (m_elf_header->e_ident[0] != '\x7f' || m_elf_header->e_ident[1] != '\x45' || m_elf_header->e_ident[2] != '\x4c' ||
        m_elf_header->e_ident[3] != '\x46')
    {
        printf("Not an ELF file magic is wrong!\n");
        return false;
    }

    return true;
}

void Elf::DebugDump() {
    printf("SELF header:\n");
    printf("  magic ..............: 0x%08" PRIx32 "\n", m_self->magic);
    printf("  version    .........: %" PRIu8 "\n", m_self->version);
    printf("  mode       .........: 0x%02" PRIx8 "\n", m_self->mode);
    printf("  endian     .........: %" PRIu8 "\n", m_self->endian);
    printf("  attributes .........: 0x%02" PRIx8 "\n", m_self->attributes);
    printf("  category   .........: 0x%02" PRIx8 "\n", m_self->category);
    printf("  program_type........: 0x%02" PRIx8 "\n", m_self->program_type);
    printf("  padding1 ...........: 0x%04" PRIx16 "\n", m_self->padding1);
    printf("  header size ........: %" PRIu16 "\n", m_self->header_size);
    printf("  meta size      .....: %" PRIu16 "\n", m_self->meta_size);
    printf("  file size ..........: %" PRIu32 "\n", m_self->file_size);
    printf("  padding2 ...........: 0x%08" PRIx32 "\n", m_self->padding2);
    printf("  segment count ......: %" PRIu16 "\n", m_self->segment_count);
    printf("  unknown 1A .........: 0x%04" PRIx16 "\n", m_self->unknown1A);
    printf("  padding3 ...........: 0x%04" PRIx16 "\n", m_self->padding3);
    printf("\n");

    printf("SELF segments:\n");

    for (int i = 0; i < m_self->segment_count; i++)
    {
        auto segment_header = m_self_segments[i];
        printf(" [%d]\n", i);
        printf("  flags ............: 0x%016" PRIx64 "\n", segment_header.flags);
        printf("  file offset ......: 0x%016" PRIx64 "\n", segment_header.file_offset);
        printf("  file size ........: %" PRIu64 "\n", segment_header.file_size);
        printf("  memory size ......: %" PRIu64 "\n", segment_header.memory_size);
    }
    printf("\n");

    printf("Elf header:\n");
    printf(" ident .........: 0x");
    for (auto i : m_elf_header->e_ident)
    {
        printf("%02x", i);
    }
    printf("\n");

    printf(" type  .........: 0x%04" PRIx16 "\n", m_elf_header->e_type);
    printf(" machine .......: 0x%04" PRIx16 "\n", m_elf_header->e_machine);
    printf(" version .......: 0x%08" PRIx32 "\n", m_elf_header->e_version);

    printf(" entry .........: 0x%016" PRIx64 "\n", m_elf_header->e_entry);
    printf(" phoff .........: 0x%016" PRIx64 "\n", m_elf_header->e_phoff);
    printf(" shoff .........: 0x%016" PRIx64 "\n", m_elf_header->e_shoff);
    printf(" flags .........: 0x%08" PRIx32 "\n", m_elf_header->e_flags);
    printf(" ehsize ........: 0x%04" PRIx16 "\n", m_elf_header->e_ehsize);
    printf(" phentsize .....: 0x%04" PRIx16 "\n", m_elf_header->e_phentsize);
    printf(" phnum .........: %" PRIu16 "\n", m_elf_header->e_phnum);
    printf(" shentsize .....: 0x%04" PRIx16 "\n", m_elf_header->e_shentsize);
    printf(" shnum .........: %" PRIu16 "\n", m_elf_header->e_shnum);
    printf(" shstrndx ......: %" PRIu16 "\n", m_elf_header->e_shstrndx);
}