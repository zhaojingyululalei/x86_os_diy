
#include "traps.h"
#include "cpu_cfg.h"
#include "../cpu.h"
#include "debug.h"
#include "cpu_instr.h"

static void dump_core_regs (exception_frame_t * frame) {
    // 打印CPU寄存器相关内容
    uint32_t esp, ss;
    /*0特权级发生异常和3特权级发生异常  栈信息略有不同*/
    if (frame->cs & 0x7) {
        ss = frame->ds;
        esp = frame->esp;
    } else {
        ss = frame->ss3;
        esp = frame->esp3;
    }

    dbg_info("IRQ: %d, error code: %d.\r\n", frame->num, frame->error_code);
    dbg_info("CS: %d\r\nDS: %d\r\nES: %d\r\nSS: %d\r\nFS:%d\r\nGS:%d\r\n",
               frame->cs, frame->ds, frame->es, ss, frame->fs, frame->gs
    );
    dbg_info("EAX:0x%x\r\n"
              "EBX:0x%x\r\n"
              "ECX:0x%x\r\n"
              "EDX:0x%x\r\n"
              "EDI:0x%x\r\n"
              "ESI:0x%x\r\n"
              "EBP:0x%x\r\n"
              "ESP:0x%x\r\n",
               frame->eax, frame->ebx, frame->ecx, frame->edx,
               frame->edi, frame->esi, frame->ebp, esp);
    dbg_info("EIP:0x%x\r\nEFLAGS:0x%x\r\n", frame->eip, frame->eflags);
}

static void do_default_handler (exception_frame_t * frame, const char * message) {
    dbg_info("--------------------------------\r\n");
    dbg_info("IRQ/Exception happend: %s.\r\n", message);
    dump_core_regs(frame);
    
    // todo: 留等以后补充打印任务栈的内容

    dbg_info("--------------------------------\r\n");
    if (frame->cs & 0x3) {
        //sys_exit(frame->error_code);
    } else {
        for (;;) {
            hlt();
        }
    }
}

void do_handler_unknown (exception_frame_t * frame) {
	do_default_handler(frame, "Unknown exception.");
}

void do_handler_divider(exception_frame_t * frame) {
	do_default_handler(frame, "Divider Error.");
}

void do_handler_Debug(exception_frame_t * frame) {
	do_default_handler(frame, "Debug Exception");
}

void do_handler_NMI(exception_frame_t * frame) {
	do_default_handler(frame, "NMI Interrupt.");
}

void do_handler_breakpoint(exception_frame_t * frame) {
	do_default_handler(frame, "Breakpoint.");
}

void do_handler_overflow(exception_frame_t * frame) {
	do_default_handler(frame, "Overflow.");
}

void do_handler_bound_range(exception_frame_t * frame) {
	do_default_handler(frame, "BOUND Range Exceeded.");
}

void do_handler_invalid_opcode(exception_frame_t * frame) {
	do_default_handler(frame, "Invalid Opcode.");
}

void do_handler_device_unavailable(exception_frame_t * frame) {
	do_default_handler(frame, "Device Not Available.");
}

void do_handler_double_fault(exception_frame_t * frame) {
	do_default_handler(frame, "Double Fault.");
}

void do_handler_invalid_tss(exception_frame_t * frame) {
	do_default_handler(frame, "Invalid TSS");
}

void do_handler_segment_not_present(exception_frame_t * frame) {
	do_default_handler(frame, "Segment Not Present.");
}

void do_handler_stack_segment_fault(exception_frame_t * frame) {
	do_default_handler(frame, "Stack-Segment Fault.");
}

void do_handler_general_protection(exception_frame_t * frame) {
    dbg_info("--------------------------------\r\n");
    dbg_info("IRQ/Exception happend: General Protection.\r\n");
    if (frame->error_code & ERR_EXT) {
        dbg_info("The exception occurred during delivery of an "
                  "event external to the program, such as an interrupt "
                  "or an earlier exception.\r\n");
    } else {
        dbg_info("The exception occurred during delivery of a "
                  "software interrupt (INT n, INT3, or INTO).\r\n");
    }
    
    if (frame->error_code & ERR_IDT) {
        dbg_info("The index portion of the error code refers "
                  "to a gate descriptor in the IDT\r\n");
    } else {
        dbg_info("The index refers to a descriptor in the GDT\r\n");
    }
    
    dbg_info("Segment index: %d\r\n", frame->error_code & 0xFFF8);

    dump_core_regs(frame);
    if (frame->cs & 0x3) {
        //sys_exit(frame->error_code);
    } else {
        for (;;) {
            hlt();
        }
    }
}

void do_handler_page_fault(exception_frame_t * frame) {
    dbg_info("--------------------------------\r\n");
    dbg_info("IRQ/Exception happend: Page fault.\r\n");
    if (frame->error_code & ERR_PAGE_P) {
        dbg_info("\tPage-level protection violation: 0x%x.\r\n", read_cr2());
    } else {
         dbg_info("\tPage doesn't present: 0x%x.\r\n", read_cr2());
    }
    
    if (frame->error_code & ERR_PAGE_WR) {
        dbg_info("\tThe access causing the fault was a read.\r\n");
    } else {
        dbg_info("\tThe access causing the fault was a write.\r\n");
    }
    
    if (frame->error_code & ERR_PAGE_US) {
        dbg_info("\tA supervisor-mode access caused the fault.\r\n");
    } else {
        dbg_info("\tA user-mode access caused the fault.\r\n");
    }

    dump_core_regs(frame);
    if (frame->cs & 0x3) {
        //sys_exit(frame->error_code);
    } else {
        for (;;) {
            hlt();
        }
    }
}

void do_handler_fpu_error(exception_frame_t * frame) {
	do_default_handler(frame, "X87 FPU Floating Point Error.");
}

void do_handler_alignment_check(exception_frame_t * frame) {
	do_default_handler(frame, "Alignment Check.");
}

void do_handler_machine_check(exception_frame_t * frame) {
	do_default_handler(frame, "Machine Check.");
}

void do_handler_smd_exception(exception_frame_t * frame) {
	do_default_handler(frame, "SIMD Floating Point Exception.");
}

void do_handler_virtual_exception(exception_frame_t * frame) {
	do_default_handler(frame, "Virtualization Exception.");
}
#include "init.h"
void interupt_install(int irq_num, irq_handler_t handler)
{
    if (irq_num >= IDT_ENTRYS_NUM || irq_num < 0)
    {
        return -1;
    }
    uint32_t handler_addr = (uint32_t)handler;
    gate_desc_t *desc = get_idt_gate_desc(irq_num);
    desc->offset_15_0 = handler_addr & 0xFFFF;
    desc->offset_31_16 = (handler_addr >> 16) & 0xFFFF;
    desc->DPL = GATE_DPL0;
    desc->p = 1;
    desc->param = 0;
    desc->selector = SELECTOR_KERNEL_CODE_SEG;
    desc->type = GATE_TRAP_TYPE_IDT;
    boot_inform->idt_entry_nr++;
    return;
}

int trap_install(int irq_num, irq_handler_t handler)
{
    if (irq_num >= IDT_ENTRYS_NUM || irq_num < 0)
    {
        return -1;
    }
    uint32_t handler_addr = (uint32_t)handler;
    gate_desc_t *desc = get_idt_gate_desc(irq_num);
    desc->offset_15_0 = handler_addr & 0xFFFF;
    desc->offset_31_16 = (handler_addr >> 16) & 0xFFFF;
    desc->DPL = GATE_DPL0;
    desc->p = 1;
    desc->param = 0;
    desc->selector = SELECTOR_KERNEL_CODE_SEG;
    desc->type = GATE_INT_TYPE_IDT;
    boot_inform->idt_entry_nr++;
    return;
}
void trap_init(void)
{

    for (uint32_t i = 0; i < IDT_ENTRYS_NUM; i++)
    {
        trap_install(i, exception_handler_unknown);
    }
    // 设置异常处理接口
    trap_install(IRQ0_DE, exception_handler_divider);
    trap_install(IRQ1_DB, exception_handler_Debug);
    trap_install(IRQ2_NMI, exception_handler_NMI);
    trap_install(IRQ3_BP, exception_handler_breakpoint);
    trap_install(IRQ4_OF, exception_handler_overflow);
    trap_install(IRQ5_BR, exception_handler_bound_range);
    trap_install(IRQ6_UD, exception_handler_invalid_opcode);
    trap_install(IRQ7_NM, exception_handler_device_unavailable);
    trap_install(IRQ8_DF, exception_handler_double_fault);
    trap_install(IRQ10_TS, exception_handler_invalid_tss);
    trap_install(IRQ11_NP, exception_handler_segment_not_present);
    trap_install(IRQ12_SS, exception_handler_stack_segment_fault);
    trap_install(IRQ13_GP, exception_handler_general_protection);
    trap_install(IRQ14_PF, exception_handler_page_fault);
    trap_install(IRQ16_MF, exception_handler_fpu_error);
    trap_install(IRQ17_AC, exception_handler_alignment_check);
    trap_install(IRQ18_MC, exception_handler_machine_check);
    trap_install(IRQ19_XM, exception_handler_smd_exception);
    trap_install(IRQ20_VE, exception_handler_virtual_exception);

}