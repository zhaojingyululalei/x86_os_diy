#include "mmu.h"
#include "../mm_block.h"
#include "cpu_cfg.h"
#include "debug.h"
#include "cpu_instr.h"
//全局页目录表
static uint32_t pgd[1024] __attribute__((aligned(4096))) = {0};



void kernel_pgd_create(void)
{
    ph_addr_t start_addr = 0;
    ph_addr_t end_addr = MEM_TOTAL_SIZE -1;
    //一张中间页表，管理4MB空间,计算需要多少张中间页表
    uint32_t pmd_num = MEM_TOTAL_SIZE / (4*1024*1024);
    //0~128M 4KB 恒等映射
    for (int i = 0; i < pmd_num; i++)
    {
        ph_addr_t pmd_addr = (uint32_t*)mm_alloc_one_page();
        //dbg_info("alloc a pmd:%x\r\n",pmd_addr);
        uint32_t* pte = (uint32_t*) pmd_addr;
        if(pmd_addr & 0xFFF != 0){
            dbg_error("pmd not align \r\n");
            return;
        }
        //给PDE赋值
        pgd[i] = pmd_addr | PDE_P | PDE_U;
        //dbg_info("pde addr %x,value %x",&pgd[i],pgd[i]);
        //给PTE赋值
        for (int j = 0; j < 1024; j++)
        {
            pte[j] = start_addr | PTE_P | PTE_W | PTE_U;
            //dbg_info("pte addr %x,value %x\r\n",&pte[j],pte[j]);
            start_addr += MEM_PAGE_SIZE;
        }
    }
    // 设置页表地址
    write_cr3((uint32_t)pgd);
    return ;
}