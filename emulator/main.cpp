#include "types.h"
#include <stdio.h>
#include <corecrt_malloc.h>
#include "Loader/Elf.h"

#pragma warning(disable:4996)





int main(int argc, char* argv[]) 
{
    const char* const path = argv[1]; //argument 1 is the path of self file to boot
    Elf* elf = new Elf;
    elf->Open(path);

}
