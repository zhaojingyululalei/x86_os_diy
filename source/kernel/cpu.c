#include "cpu.h"
#include "types.h"
#include "cpu_instr.h"
#include "cpu_cfg.h"
#include "irq/irq.h"

seg_desc_t* gdt;
gate_desc_t idt[IDT_ENTRYS_NUM];

static void idt_init(void)
{
    boot_inform->idt_base_addr = (uint32_t)idt;
    boot_inform->idt_entry_nr = 0;
    boot_inform->idt_entry_limit = IDT_ENTRYS_NUM;

    trap_init();
    lidt((uint32_t)idt,IDTR_LIMIT);
}

gate_desc_t* get_idt_gate_desc(int irq_num)
{
    return &idt[irq_num];
}



void irq_disable_global(void) {
    cli();
}

void irq_enable_global(void) {
    sti();
}

/**
 * @brief 进入中断保护
 */
irq_state_t irq_enter_protection (void) {
    irq_state_t state = read_eflags();
    irq_disable_global();
    return state;
}

/**
 * @brief 退出中断保护
 */
void irq_leave_protection (irq_state_t state) {
    write_eflags(state);
}


void cpu_init(void)
{
    gdt = (seg_desc_t*)(boot_inform->gdt_base_addr);
    idt_init();
    init_pic();
}