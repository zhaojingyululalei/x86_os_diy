

  	# 16位代码，务必加上
  	.code16
#include "cpu_cfg.h"
 	.text
    .extern detect_memory
	.global _start
_start:

# 重置数据段寄存器
	mov $BOOTLOADER_SEG, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	mov $STACK_KERNEL_TOP_ADDR,%sp


	mov $0, %si
    mov $loader_msg,%si
    call print

    # 检测内存
    push $0            # 这里push2字节，作为ip地址高位
    call detect_memory   # 这里push2字节的ip

    # 进入32位保护模式

    cli # 关中断

    lgdt gdt_info
    lidt idt_info
    # 开A20
    inb $0x92, %al
    or $0x02, %al
    outb %al, $0x92

    # 设置保护模式
    mov %cr0, %eax
    or $1, %eax
    mov %eax, %cr0

    ljmpl $SELECTOR_BOOTLOADER_CODE_SEG,$protected_mode

    
    jmp .






print:
    mov $0x0e, %ah       # BIOS: 0x10 中断，0x0E 功能号（TTY 输出）
    mov $0x01, %bl       # 字符颜色（默认亮白）

print_loop:
    lodsb                # 加载 (%SI) 到 AL，SI 自动增加
    cmp $0, %al          # 判断是否到字符串末尾
    jz print_done        # 如果 AL == 0，则结束打印

    cmp $'\n', %al       # 检测换行符 '\n'
    je print_newline     # 如果是换行符，跳转处理

    int $0x10            # 通过 BIOS 中断打印字符
    jmp print_loop       # 打印下一个字符

print_newline:
    mov $'\r', %al       # 回车 '\r'
    int $0x10            # 打印回车符
    mov $'\n', %al       # 换行 '\n'
    int $0x10            # 打印换行符
    jmp print_loop       # 继续打印下一个字符

print_done:
    ret


   
loader_msg: .asciz "enter loader successful!\n"

_gdt:
    .long 0,0
bootloader_code_desc:
    .long ((DESC_BOOTLOADER_SEG_LIMIT & 0xffff) << GDT_DESC_LIMIT_0_15_POS) | \
            ((DESC_BOOTLOADER_BASE_ADDR & 0xffff) << GDT_DESC_BASE_0_15_POS)
    .long (((DESC_BOOTLOADER_BASE_ADDR >> 16) & 0xff) << GDT_DESC_BASE_16_23_POS) | \
            (0b1000 << GDT_DESC_TYPE_POS) | \
            (0b1 << GDT_DESC_S_POS) | \
            (0b00 << GDT_DESC_DPL_POS) | \
            (0b1 << GDT_DESC_P_POS) | \
            ((DESC_BOOTLOADER_SEG_LIMIT >> 16 & 0xf) << GDT_DESC_LIMIT_16_19_POS) | \
            (0b00 << GDT_DESC_AVL_POS) | \
            (0b1 << GDT_DESC_DB_POS) | \
            (0b1 << GDT_DESC_G_POS) | \
            ((DESC_BOOTLOADER_BASE_ADDR >> 24) & 0xff) << GDT_DESC_BASE_24_31_POS

bootloader_data_desc:
    .long ((DESC_BOOTLOADER_SEG_LIMIT & 0xffff) << GDT_DESC_LIMIT_0_15_POS) | \
            ((DESC_BOOTLOADER_BASE_ADDR & 0xffff) << GDT_DESC_BASE_0_15_POS)
    
    .long (((DESC_BOOTLOADER_BASE_ADDR >> 16) & 0xff) << GDT_DESC_BASE_16_23_POS) | \
            (0b0010 << GDT_DESC_TYPE_POS) | \
            (0b1 << GDT_DESC_S_POS) | \
            (0b00 << GDT_DESC_DPL_POS) | \
            (0b1 << GDT_DESC_P_POS) | \
            ((DESC_BOOTLOADER_SEG_LIMIT >> 16 & 0xf) << GDT_DESC_LIMIT_16_19_POS) | \
            (0b00 << GDT_DESC_AVL_POS) | \
            (0b1 << GDT_DESC_DB_POS) | \
            (0b1 << GDT_DESC_G_POS) | \
            ((DESC_BOOTLOADER_BASE_ADDR >> 24) & 0xff) << GDT_DESC_BASE_24_31_POS

kernel_code_desc:
    .long ((DESC_KERNEL_CODE_SEG_LIMIT & 0xffff) << GDT_DESC_LIMIT_0_15_POS) | \
           ((DESC_KERNEL_CODE_BASE_ADDR & 0xffff) << GDT_DESC_BASE_0_15_POS)
    .long (((DESC_KERNEL_CODE_BASE_ADDR >> 16) & 0xff) << GDT_DESC_BASE_16_23_POS) | \
           (0b1000 << GDT_DESC_TYPE_POS) | \
           (0b1 << GDT_DESC_S_POS) | \
           (0b00 << GDT_DESC_DPL_POS) | \
           (0b1 << GDT_DESC_P_POS) | \
           ((DESC_KERNEL_CODE_SEG_LIMIT >> 16 & 0xf) << GDT_DESC_LIMIT_16_19_POS) | \
           (0b00 << GDT_DESC_AVL_POS) | \
           (0b1 << GDT_DESC_DB_POS) | \
           (0b1 << GDT_DESC_G_POS) | \
           ((DESC_KERNEL_CODE_BASE_ADDR >> 24) & 0xff) << GDT_DESC_BASE_24_31_POS

kernel_data_desc:
    .long ((DESC_KERNEL_DATA_SEG_LIMIT & 0xffff) << GDT_DESC_LIMIT_0_15_POS) | \
           ((DESC_KERNEL_DATA_BASE_ADDR & 0xffff) << GDT_DESC_BASE_0_15_POS)
    .long (((DESC_KERNEL_DATA_BASE_ADDR >> 16) & 0xff) << GDT_DESC_BASE_16_23_POS) | \
           (0b0010 << GDT_DESC_TYPE_POS) | \
           (0b1 << GDT_DESC_S_POS) | \
           (0b00 << GDT_DESC_DPL_POS) | \
           (0b1 << GDT_DESC_P_POS) | \
           ((DESC_KERNEL_DATA_SEG_LIMIT >> 16 & 0xf) << GDT_DESC_LIMIT_16_19_POS) | \
           (0b00 << GDT_DESC_AVL_POS) | \
           (0b1 << GDT_DESC_DB_POS) | \
           (0b1 << GDT_DESC_G_POS) | \
           ((DESC_KERNEL_DATA_BASE_ADDR >> 24) & 0xff) << GDT_DESC_BASE_24_31_POS

usr_code_desc:
    .long ((DESC_USR_CODE_SEG_LIMIT & 0xffff) << GDT_DESC_LIMIT_0_15_POS) | \
           ((DESC_USR_CODE_BASE_ADDR & 0xffff) << GDT_DESC_BASE_0_15_POS)
    .long (((DESC_USR_CODE_BASE_ADDR >> 16) & 0xff) << GDT_DESC_BASE_16_23_POS) | \
           (0b1000 << GDT_DESC_TYPE_POS) | \
           (0b1 << GDT_DESC_S_POS) | \
           (0b11 << GDT_DESC_DPL_POS) | \
           (0b1 << GDT_DESC_P_POS) | \
           ((DESC_USR_CODE_SEG_LIMIT >> 16 & 0xf) << GDT_DESC_LIMIT_16_19_POS) | \
           (0b00 << GDT_DESC_AVL_POS) | \
           (0b1 << GDT_DESC_DB_POS) | \
           (0b1 << GDT_DESC_G_POS) | \
           ((DESC_USR_CODE_BASE_ADDR >> 24) & 0xff) << GDT_DESC_BASE_24_31_POS

usr_data_desc:
    .long ((DESC_USR_DATA_SEG_LIMIT & 0xffff) << GDT_DESC_LIMIT_0_15_POS) | \
           ((DESC_USR_DATA_BASE_ADDR & 0xffff) << GDT_DESC_BASE_0_15_POS)
    .long (((DESC_USR_DATA_BASE_ADDR >> 16) & 0xff) << GDT_DESC_BASE_16_23_POS) | \
           (0b0010 << GDT_DESC_TYPE_POS) | \
           (0b1 << GDT_DESC_S_POS) | \
           (0b11 << GDT_DESC_DPL_POS) | \
           (0b1 << GDT_DESC_P_POS) | \
           ((DESC_USR_DATA_SEG_LIMIT >> 16 & 0xf) << GDT_DESC_LIMIT_16_19_POS) | \
           (0b00 << GDT_DESC_AVL_POS) | \
           (0b1 << GDT_DESC_DB_POS) | \
           (0b1 << GDT_DESC_G_POS) | \
           ((DESC_USR_DATA_BASE_ADDR >> 24) & 0xff) << GDT_DESC_BASE_24_31_POS

gdt_info:
    .word 0x800-1
    .word _gdt+0,0  # 16位地址，高16位肯定是0

_idt:
idt_info:
    .word 0
    .word 0,0  # 此时关中断



    .code32
    .text
    .extern load_kernel,boot_info
protected_mode:
    mov $SELECTOR_BOOTLOADER_DATA_SEG,%ax
    mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

    mov $STACK_KERNEL_TOP_ADDR,%esp

    mov $_gdt,%eax
    movl %eax,boot_info

    mov $7,%eax
    movl %eax,boot_info+4

    mov $256,%eax
    movl %eax,boot_info+8

    mov $0,%eax
    movl %eax,boot_info+12
    
    mov $0,%eax
    movl %eax,boot_info+16

    mov $256,%eax
    movl %eax,boot_info+20

    pushl $boot_info
    call load_kernel

    jmp .

