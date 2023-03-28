#include "types.h"
#include <stdio.h>
#include <corecrt_malloc.h>
#include "Loader/Elf.h"

#pragma warning(disable:4996)





int main(int argc, char* argv[]) 
{
    const char* const path = argv[1]; //argument 1 is the path of self file to boot

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

    

    Elf* elf = new Elf;
    elf->Open(path);

    

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
