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

int gdt_alloc_desc(void)
{
    int ret_sel,i;
    for (i = 1; i < GDT_ENTRYS_NUM; i++) {
        seg_desc_t * desc = gdt + i;
        if (desc->p == 0) {
            desc->p = 1;     // 标记为占用状态
            ret_sel = i << 3;
            break;
        }
    }
    if(i>=GDT_ENTRYS_NUM)
    {
        return -1;
    }
    boot_inform->gdt_entry_nr++;
    return ret_sel;
}
int gdt_free_desc(int selector){
    int idx = selector >> 3;
    gdt[idx].p = 0;
    return 0;
}
/*
*安装tss段
*/
void gdt_set_tss(int tss_sel,ph_addr_t tss_base,uint32_t tss_limit)
{
    seg_desc_t* g = gdt+(tss_sel>>3);
    g->p = 1;
    g->D_B = 1;
    g->type = SEG_TYPE_TSS;
    g->base_15_0 = tss_base & 0xFFFF;
    g->base_23_16 = (tss_base >> 16) & 0xFF;
    g->base_31_24 = (tss_base >> 24) & 0xFF;

    g->limit_15_0 = tss_limit & 0xFFFF;
    g->limit_19_6 = (tss_limit >> 16)&0xF;
}

void gdt_init(void)
{
    for(int i = boot_inform->gdt_entry_nr;i<GDT_ENTRYS_NUM;++i)
    {
        gdt[i].v_low=0;
        gdt[i].v_high=0;
    }
}
void cpu_init(void)
{
    gdt = (seg_desc_t*)(boot_inform->gdt_base_addr);
    gdt_init();
    idt_init();
    init_pic();
}