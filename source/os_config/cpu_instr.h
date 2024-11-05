#ifndef __CPU_INSTR_H
#define __CPU_INSTR_H

#include "types.h"


static inline uint8_t inb(uint16_t  port) {
	uint8_t rv;
	__asm__ __volatile__("inb %[p], %[v]" : [v]"=a" (rv) : [p]"d"(port));
	return rv;
}

static inline uint16_t inw(uint16_t  port) {
	uint16_t rv;
	__asm__ __volatile__("in %1, %0" : "=a" (rv) : "dN" (port));
	return rv;
}

static inline void outb(uint16_t port, uint8_t data) {
	__asm__ __volatile__("outb %[v], %[p]" : : [p]"d" (port), [v]"a" (data));
}

static inline uint32_t read_cr0() {
	uint32_t cr0;
	__asm__ __volatile__("mov %%cr0, %[v]":[v]"=r"(cr0));
	return cr0;
}

static inline void write_cr0(uint32_t v) {
	__asm__ __volatile__("mov %[v], %%cr0"::[v]"r"(v));
}

static inline uint32_t read_cr2() {
	uint32_t cr2;
	__asm__ __volatile__("mov %%cr2, %[v]":[v]"=r"(cr2));
	return cr2;
}

static inline void write_cr3(uint32_t v) {
    __asm__ __volatile__("mov %[v], %%cr3"::[v]"r"(v));
}

static inline uint32_t read_cr3() {
    uint32_t cr3;
    __asm__ __volatile__("mov %%cr3, %[v]":[v]"=r"(cr3));
    return cr3;
}

static inline uint32_t read_cr4() {
    uint32_t cr4;
    __asm__ __volatile__("mov %%cr4, %[v]":[v]"=r"(cr4));
    return cr4;
}

static inline void write_cr4(uint32_t v) {
    __asm__ __volatile__("mov %[v], %%cr4"::[v]"r"(v));
}

#endif
