#ifndef __CPU_H
#define __CPU_H
#include "types.h"
#include "init.h"

#define GATE_INT_TYPE_IDT (0xE)     // 中断32位门描述符
#define GATE_TRAP_TYPE_IDT (0xF)    // 中断32位门描述符
#define GATE_SYSCALL_TYPE_GDT (0xC) // 调用门
#define GATE_P_PRESENT (1)          // 是否存在
#define GATE_DPL0 (0)               // 特权级0，最高特权级
#define GATE_DPL3 (3)               // 特权级3，最低权限

#define SEG_TYPE_TSS (9) // 32位TSS

#define EFLAGS_IF (1 << 9)
#define EFLAGS_DEFAULT (1 << 1)

#pragma pack(1)
// 段描述符
typedef union _seg_desc_t
{
    struct
    {
        uint32_t v_low;
        uint32_t v_high;
    };
    struct
    {
        uint16_t limit_15_0;
        uint16_t base_15_0;
        uint8_t base_23_16;
        uint8_t type : 4;
        uint8_t s : 1;
        uint8_t DPL : 2;
        uint8_t p : 1;
        uint8_t limit_19_6 : 4;
        uint8_t AVL : 1;
        uint8_t L : 1;
        uint8_t D_B : 1;
        uint8_t G : 1;
        uint8_t base_31_24;
    };
} seg_desc_t;

// 门描述符
typedef union _gate_desc_t
{
    struct
    {
        uint32_t v_low;
        uint32_t v_high;
    };

    struct
    {
        uint16_t offset_15_0;
        uint16_t selector;
        uint8_t param; // 参数,注：调用门参数个数最多31个
        uint8_t type : 5;
        uint8_t DPL : 2;
        uint8_t p : 1;
        uint16_t offset_31_16;
    };

} gate_desc_t;

#pragma pack()
typedef uint32_t irq_state_t;
void irq_disable_global(void);
void irq_enable_global(void);
irq_state_t irq_enter_protection(void);
void irq_leave_protection(irq_state_t state);

void cpu_init(void);
gate_desc_t *get_idt_gate_desc(int irq_num);
int gdt_set_tss(int tss_sel,ph_addr_t tss_base,uint32_t tss_limit);
int gdt_set_call(int call_sel,int selector,int parm_count,ph_addr_t base);
int gdt_alloc_desc(void);
int gdt_free_desc(int selector);
#endif
