#ifndef __CPU_H
#define __CPU_H
#include "types.h"
typedef uint32_t irq_state_t;


void irq_disable_global(void);

void irq_enable_global(void);
irq_state_t irq_enter_protection (void);
void irq_leave_protection (irq_state_t state);
#endif
