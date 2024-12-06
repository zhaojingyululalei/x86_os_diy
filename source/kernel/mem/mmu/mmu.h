#ifndef __MMU_H
#define __MMU_H

#include "types.h"
#define PTE_P       (1 << 0)
#define PTE_W       (1 << 1)
#define PTE_U       (1 << 2)

#define PDE_P       (1 << 0)
#define PDE_W       (1 << 1)
#define PDE_U       (1 << 2)
#pragma pack(1)
/**
 * @brief Page-Table Entry
 */
typedef union _pde_t {
    uint32_t v;
    struct {
        uint32_t present : 1;                   // 0 (P) Present; must be 1 to map a 4-KByte page
        uint32_t write_disable : 1;             // 1 (R/W) Read/write, if 0, writes may not be allowe
        uint32_t user_mode_acc : 1;             // 2 (U/S) if 0, user-mode accesses are not allowed t
        uint32_t write_through : 1;             // 3 (PWT) Page-level write-through
        uint32_t cache_disable : 1;             // 4 (PCD) Page-level cache disable
        uint32_t accessed : 1;                  // 5 (A) Accessed
        uint32_t : 1;                           // 6 Ignored;
        uint32_t ps : 1;                        // 7 (PS) 0 4KB  1 4MB
        uint32_t : 4;                           // 11:8 Ignored
        uint32_t phy_pt_addr : 20;              // 高20位page table物理地址
    };
}pde_t;

#define PTE_IGNORE_COPY_ON_WRITE    1
/**
 * @brief Page-Table Entry
 */
typedef union _pte_t {
    uint32_t v;
    struct {
        uint32_t present : 1;                   // 0 (P) Present; must be 1 to map a 4-KByte page
        uint32_t write_disable : 1;             // 1 (R/W) Read/write, if 0, writes may not be allowe
        uint32_t user_mode_acc : 1;             // 2 (U/S) if 0, user-mode accesses are not allowed t
        uint32_t write_through : 1;             // 3 (PWT) Page-level write-through
        uint32_t cache_disable : 1;             // 4 (PCD) Page-level cache disable
        uint32_t accessed : 1;                  // 5 (A) Accessed;
        uint32_t dirty : 1;                     // 6 (D) Dirty
        uint32_t pat : 1;                       // 7 PAT
        uint32_t global : 1;                    // 8 (G) Global
        uint32_t ignore : 3;                    // Ignored
        uint32_t phy_page_addr : 20;            // 高20位物理地址
    };
}pte_t;
#pragma pack()


/**
 * @brief 返回vaddr在页目录中的索引
 */
static inline uint32_t pde_index (uint32_t vaddr) {
    int index = (vaddr >> 22); // 只取高10位
    return index;
}

/**
 * @brief 返回vaddr在页表中的索引
 */
static inline int pte_index (uint32_t vaddr) {
    return (vaddr >> 12) & 0x3FF;   // 取中间10位
}

void mmu_test(void);
//内核代码不用释放
void kernel_pgd_create(void);
int mmu_memory_map(pde_t page_dir[] ,ph_addr_t vm, ph_addr_t phm, uint32_t write_disable, uint32_t user_mode_acc);
ph_addr_t mmu_get_phaddr(pde_t page_dir[],ph_addr_t vm) ;

//浅拷贝内核页表，因为内核区一模一样
ph_addr_t mmu_create_task_pgd(void);
int mmu_destory_task_pgd(pde_t page_dir[]);

pte_t* mmu_from_vm_get_pte(pde_t page_dir[],ph_addr_t vm);
pde_t *mmu_from_vm_get_pde(pde_t page_dir[],ph_addr_t vm);

//深拷贝
void mmu_cpy_page_dir(pde_t from[], pde_t to[], ph_addr_t start_vm, int page_count);



#endif
