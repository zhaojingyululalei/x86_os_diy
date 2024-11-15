#include "memory.h"

void memory_init(void)
{
    memblock_init();
    kernel_pgd_create();
}