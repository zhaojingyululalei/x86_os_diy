#include "mmu.h"
#include "../mm_block.h"
#include "cpu_cfg.h"
#include "debug.h"
#include "cpu_instr.h"
// 全局页目录表
static uint32_t pgd[1024] __attribute__((aligned(4096))) = {0};

void kernel_pgd_create(void)
{
    ph_addr_t start_addr = 0;
    ph_addr_t end_addr = MEM_TOTAL_SIZE - 1;
    // 一张中间页表，管理4MB空间,计算需要多少张中间页表
    uint32_t pmd_num = MEM_TOTAL_SIZE / (4 * 1024 * 1024);
    // 0~128M 4KB 恒等映射
    for (int i = 0; i < pmd_num; i++)
    {
        ph_addr_t pmd_addr = (uint32_t *)mm_alloc_one_page();
        // dbg_info("alloc a pmd:%x\r\n",pmd_addr);
        uint32_t *pte = (uint32_t *)pmd_addr;
        if (pmd_addr & 0xFFF != 0)
        {
            dbg_error("pmd not align \r\n");
            return;
        }
        // 给PDE赋值
        pgd[i] = pmd_addr | PDE_P | PDE_U | PDE_W;
        // dbg_info("pde addr %x,value %x",&pgd[i],pgd[i]);
        // 给PTE赋值
        for (int j = 0; j < 1024; j++)
        {
            pte[j] = start_addr | PTE_P | PTE_W;
            // dbg_info("pte addr %x,value %x\r\n",&pte[j],pte[j]);
            start_addr += MEM_PAGE_SIZE;
        }
    }
    // 设置页表地址
    write_cr3((uint32_t)pgd);
    return;
}

/*建立虚拟地址，物理地址内存映射关系*/
int mmu_memory_map(pde_t page_dir[], ph_addr_t vm, ph_addr_t phm, uint32_t write_disable, uint32_t user_mode_acc)
{
    int pgd_idx = (vm >> 22) & 0x3FF; // 取虚拟地址高10位
    pde_t *pde = &page_dir[pgd_idx];
    ph_addr_t pmd, alloced;
    if (!pde->present)
    {
        pmd = mm_alloc_one_page();
        alloced = pmd;
        pde->present = 1;
        pde->write_disable = 1;
        pde->user_mode_acc = 1;
        pde->phy_pt_addr = pmd >> 12;
    }
    else
    {
        pmd = pde->phy_pt_addr << 12;
    }
    pte_t *pte = (pte_t *)pmd;

    int pmd_idx = (vm >> 12) & 0x3FF; // 取虚拟地址中间10位
    pte += pmd_idx;
    if (pte->present)
    {
        mm_free_one_page(alloced);
        dbg_error("There is a mapping relationship here, and this operation will overwrite the existing mapping\r\n");
        return -1;
    }
    pte->present = 1;
    pte->write_disable = write_disable;
    pte->user_mode_acc = user_mode_acc;
    pte->phy_page_addr = phm >> 12;

    return 1;
}

/*根据虚拟地址获取物理地址*/
ph_addr_t mmu_get_phaddr(pde_t page_dir[], ph_addr_t vm)
{
    int pgd_idx = (vm >> 22) & 0x3FF; // 获取虚拟地址的页目录索引（高10位）
    int pmd_idx = (vm >> 12) & 0x3FF; // 获取虚拟地址的页表索引（中间10位）
    int page_offset = vm & 0xFFF;     // 获取虚拟地址的页内偏移（低12位）

    // 获取页目录项
    pde_t *pde = &page_dir[pgd_idx];
    if (!pde->present)
    {
        dbg_error("Page directory entry not present");
        return 0; // 页目录项不存在
    }

    // 获取页表基地址
    ph_addr_t pmd = pde->phy_pt_addr << 12;
    pte_t *pte = (pte_t *)pmd;

    // 获取页表项
    pte_t *page_entry = &pte[pmd_idx];
    if (!page_entry->present)
    {
        dbg_error("Page table entry not present");
        return 0;
    }

    ph_addr_t physical_page = page_entry->phy_page_addr << 12;
    ph_addr_t physical_address = physical_page | page_offset;

    return physical_address;
}
#include "string.h"
/**
 * 创建任务页表，并且拷贝内核页表
 */
ph_addr_t mmu_create_task_pgd(void)
{
    // 分配一个页目录表
    ph_addr_t page_dir = mm_alloc_one_page();
    if (page_dir < 0)
    {
        return 0;
    }
    // 然后就是拷贝内核页表
    pde_t *pde_dest = (pde_t *)page_dir;
    pde_t *pde_src = (pde_t *)pgd;
    for (int i = 0; i < 1024; i++)
    {
        if (pde_src[i].present)
        {
            pde_dest[i].v = pde_src[i].v;
            ph_addr_t page_table = mm_alloc_one_page();
            pde_dest[i].phy_pt_addr = page_table >> 12;

            pte_t *pte_dest = (pte_t *)page_table;
            pte_t *pte_src = (pte_t *)(pde_src[i].phy_pt_addr << 12);
            for (int i = 0; i < 1024; ++i)
            {
                if (pte_src[i].present)
                {
                    pte_dest[i].v = pte_src[i].v;
                }
            }
        }
    }
    return page_dir;
}
int mmu_destory_task_pgd(pde_t page_dir[])
{
    int ret = 0;
    pde_t *pde_cur = page_dir;
    for (int i = 0; i < 1024; i++, pde_cur++)
    {
        if (pde_cur->present)
        {
            pte_t *pte_cur = (pte_t *)(pde_cur->phy_pt_addr << 12);
            for (int j = 0; j < 1024; ++j, pte_cur++)
            {
                if (pte_cur->present)
                {
                    ph_addr_t phm = pte_cur->phy_page_addr << 12;
                    ret = mm_free_one_page(phm);
                    if (ret < 0)
                    {
                        return ret;
                    }
                    pte_cur->v = 0;
                }
            }
            ret = mm_free_one_page((ph_addr_t)(pde_cur->phy_pt_addr << 12));
            if (ret < 0)
            {
                return ret;
            }
            pde_cur->v = 0;
        }
    }
    return 0;
}
pte_t *mmu_from_vm_get_pte(pde_t page_dir[], ph_addr_t vm)
{
    if (page_dir == NULL)
    {
        dbg_error("there is no pagedir\r\n");
        return NULL;
    }
    int pgd_idx = (vm >> 22) & 0x3FF; // 获取虚拟地址的页目录索引（高10位）
    int pmd_idx = (vm >> 12) & 0x3FF; // 获取虚拟地址的页表索引（中间10位）

    // 获取页目录项
    pde_t *pde = &page_dir[pgd_idx];

    // 获取页表基地址
    ph_addr_t pmd = pde->phy_pt_addr << 12;
    pte_t *pte = (pte_t *)pmd;

    // 获取页表项
    pte_t *page_entry = &pte[pmd_idx];

    return page_entry;
}

pde_t *mmu_from_vm_get_pde(pde_t page_dir[], ph_addr_t vm)
{
    if (page_dir == NULL)
    {
        dbg_info("there is no page_dir\r\n");
        return NULL;
    }
    int pgd_idx = (vm >> 22) & 0x3FF; // 获取虚拟地址的页目录索引（高10位）

    // 获取页目录项
    pde_t *pde = &page_dir[pgd_idx];
    return pde;
}
void mmu_cpy_page_dir(pde_t from[], pde_t to[], ph_addr_t start_vm, int page_count)
{
    pde_t *pde_src = mmu_from_vm_get_pde(from, start_vm);
    pde_t *pde_dest = mmu_from_vm_get_pde(to, start_vm);
    ph_addr_t vm = start_vm;
    while (page_count)
    {
        if (pde_src->present)
        {
            pde_dest->v = pde_src->v;
            ph_addr_t page_table = mm_alloc_one_page();
            pde_dest->phy_pt_addr = page_table >> 12;

            for (;;)
            {
                pte_t *pte_dest = mmu_from_vm_get_pte(to, vm);
                pte_t *pte_src = mmu_from_vm_get_pte(from, vm);
                if (pte_src->present)
                {
                    pte_dest->v = pte_src->v;
                    ph_addr_t src_ph = pte_src->phy_page_addr << 12;
                    ph_addr_t dest_ph = mm_alloc_one_page();
                    memcpy((uint8_t *)dest_ph, (uint8_t *)src_ph, MEM_PAGE_SIZE);
                    pte_dest->phy_page_addr = dest_ph >> 12;
                    page_count--;
                    if (page_count == 0)
                    {
                        break;
                    }
                }
                vm += MEM_PAGE_SIZE;
            }
        }
    }
    return;
}

void mmu_test(void)
{
    // // 测试虚拟地址和物理地址
    // ph_addr_t vm1 = 0xFFFCC000; // 虚拟地址
    // ph_addr_t phm1 = 0x200000;  // 物理地址

    // // 建立映射
    // int result = mmu_memory_map(pgd,vm1, phm1, 1, 0); // 可写，用户模式访问
    // if (result == 1) {
    //     dbg_info("Mapping successful: VM 0x%x -> PM 0x%x\r\n", vm1, phm1);
    // } else {
    //     dbg_info("Mapping failed for VM 0x%x -> PM 0x%x\r\n", vm1, phm1);
    // }

    // // 获取物理地址
    // ph_addr_t phm1_test = mmu_get_phaddr(vm1);
    // if (phm1_test == -1) {
    //     dbg_info("Failed to get physical address for VM 0x%x\r\n", vm1);
    // } else {
    //     dbg_info("Physical address for VM 0x%x: PM 0x%x\r\n", vm1, phm1_test);
    // }

    // // 验证映射是否正确
    // if (phm1_test == phm1) {
    //     dbg_info("Mapping verification successful: VM 0x%x -> PM 0x%x\r\n", vm1, phm1);
    // } else {
    //     dbg_info("Mapping verification failed: Expected PM 0x%x, Got PM 0x%x\r\n", phm1, phm1_test);
    // }
}