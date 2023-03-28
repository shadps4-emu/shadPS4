#include "types.h"
#include <stdio.h>
#include <corecrt_malloc.h>
#include "Loader/Elf.h"

#pragma warning(disable:4996)





int main(int argc, char* argv[]) 
{
    const char* const path = "eboot.bin";//argv[1]; //argument 1 is the path of self file to boot

    auto handle = fopen(path, "rb");
    if (handle == nullptr)
    {
        printf("Failed to open file.\n");
        return 2;
    }

    self_header header;
    if (fread(&header, sizeof(self_header), 1, handle) != 1)
    {
        printf("Failed to read SELF header.\n");
        fclose(handle);
        return 3;
    }

    if (header.magic != self_header::signature)
    {
        printf("Not a SELF file.\n");
        fclose(handle);
        return 4;
    }

    Elf* elf = new Elf;
    elf->Open(path);

    printf("SELF header:\n");
    printf("  magic ..............: 0x%08X\n", header.magic);
    printf("  version    .........: %d\n", header.version);
    printf("  mode       .........: 0x%X\n", header.mode);
    printf("  endian     .........: %d\n", header.endian);
    printf("  attributes .........: 0x%X\n", header.attributes);
    printf("  category   .........: 0x%X\n", header.category);
    printf("  program_type........: 0x%X\n", header.program_type);
    printf("  header size ........: 0x%X\n", header.header_size);
    printf("  meta size      .....: 0x%X\n", header.meta_size);
    printf("  file size ..........: 0x%X\n", header.file_size);
    printf("  padding2 ...........: 0x%08X\n", header.padding2);
    printf("  segment count ......: %u\n", header.segment_count);
    printf("  unknown 1A .........: 0x%04X\n", header.unknown1A);
    printf("  padding3 ...........: 0x%04X\n", header.padding3);
    printf("\n");

    auto segment_headers = (self_segment_header*)malloc(sizeof(self_segment_header) * header.segment_count);
    if (fread(segment_headers, sizeof(self_segment_header), header.segment_count, handle) != header.segment_count)
    {
        printf("Failed to read SELF segment headers.\n");
        free(segment_headers);
        fclose(handle);
        return 5;
    }

    printf("SELF segments:\n");

    for (int i = 0; i < header.segment_count; i++)
    {
        auto segment_header = segment_headers[i];
        printf(" [%d]\n", i);
        printf("  flags ............: %llx\n", segment_header.flags);
        printf("  offset ...........: %llx\n", segment_header.offset);
        printf("  compressed size ..: %llx\n", segment_header.encrypted_compressed_size);
        printf("  uncompressed size : %llx\n", segment_header.decrypted_decompressed_size);
    }
    printf("\n");

}
