#ifndef __TRAPS_H
#define __TRAPS_H

typedef void (*irq_handler_t)(void);

/*中断号码*/ 
//默认异常
#define IRQ0_DE             0
#define IRQ1_DB             1
#define IRQ2_NMI            2
#define IRQ3_BP             3
#define IRQ4_OF             4
#define IRQ5_BR             5
#define IRQ6_UD             6
#define IRQ7_NM             7
#define IRQ8_DF             8
#define IRQ10_TS            10
#define IRQ11_NP            11
#define IRQ12_SS            12
#define IRQ13_GP            13
#define IRQ14_PF            14
#define IRQ16_MF            16
#define IRQ17_AC            17
#define IRQ18_MC            18
#define IRQ19_XM            19
#define IRQ20_VE            20

//外部中断,来自8259
#define IRQ0_TIMER          0x20

//错误码
#define ERR_PAGE_P          (1 << 0)
#define ERR_PAGE_WR          (1 << 1)
#define ERR_PAGE_US          (1 << 1)

#define ERR_EXT             (1 << 0)
#define ERR_IDT             (1 << 1)

#pragma pack(1)
// 异常信息栈结构
typedef struct _exception_frame_t {
    // 结合压栈的过程，以及pusha指令的实际压入过程
    int gs, fs, es, ds;
    int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    int num;
    int error_code;
    int eip, cs, eflags;
    int esp3, ss3;
}exception_frame_t;


#pragma pack()



//中断处理函数
extern void exception_handler_unknown (void);
extern void exception_handler_divider (void);
extern void exception_handler_Debug (void);
extern void exception_handler_NMI (void);
extern void exception_handler_breakpoint (void);
extern void exception_handler_overflow (void);
extern void exception_handler_bound_range (void);
extern void exception_handler_invalid_opcode (void);
extern void exception_handler_device_unavailable (void);
extern void exception_handler_double_fault (void);
extern void exception_handler_invalid_tss (void);
extern void exception_handler_segment_not_present (void);
extern void exception_handler_stack_segment_fault (void);
extern void exception_handler_general_protection (void);
extern void exception_handler_page_fault (void);
extern void exception_handler_fpu_error (void);
extern void exception_handler_alignment_check (void);
extern void exception_handler_machine_check (void);
extern void exception_handler_smd_exception (void);
extern void exception_handler_virtual_exception (void);


void interupt_install(int irq_num, irq_handler_t handler);
int trap_install(int irq_num, irq_handler_t handler);
void trap_init(void);
#endif

