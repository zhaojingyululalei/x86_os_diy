ENTRY(_start);
SECTIONS
{
    /*内核区*/
    . = 0x100000;
    /*只读*/
    .text : {
		*(EXCLUDE_FILE(*first_task*) .text)
	}
	.rodata : {
		*(EXCLUDE_FILE(*first_task*) .rodata)
	}

    /*可写*/
    . = ALIGN(4096);
    .data : {
		*(EXCLUDE_FILE(*first_task*) .data)
	}
	.bss : {
		*(EXCLUDE_FILE(*first_task*) .bss)
	}

    e_kernel = .;

    /*初始进程区*/
    . = 0x80000000;
    /*LOADDR--物理地址   ADDR---虚拟地址*/
    PROVIDE(s_first_task_ph = LOADADDR(.first_task));
    PROVIDE(s_first_task_vm = ADDR(.first_task));
    .first_task : AT(e_kernel) {
		*first_task*(.text .data. rodata .data)
	}
    PROVIDE(e_first_task_ph = LOADADDR(.first_task) + SIZEOF(.first_task));
    PROVIDE(e_first_task_vm = ADDR(.first_task) + SIZEOF(.first_task));


}