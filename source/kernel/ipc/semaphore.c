#include "semaphor.h"

void sys_sem_init (sem_t * sem, int init_count) {
    sem->count = init_count;
    list_init(&sem->wait_list);
}


void sys_sem_wait (sem_t * sem) {
    irq_state_t  irq_state = irq_enter_protection();

    if (sem->count > 0) {
        sem->count--;
    } else {
        // 从就绪队列中移除，然后加入信号量的等待队列
        task_t * curr = get_cur_task();
        list_insert_last(&sem->wait_list, &curr->node);
        set_cur_task(NULL);
        schedul();
    }

    irq_leave_protection(irq_state);
}

void sys_sem_notify (sem_t * sem) {
    irq_state_t  irq_state = irq_enter_protection();

    if (list_count(&sem->wait_list)) {
        // 有进程等待，则唤醒加入就绪队列
        list_node_t * fnode = list_remove_first(&sem->wait_list);
        task_t * task = list_node_parent(fnode, task_t, node);
        set_task_to_ready_list(task);

        schedul();
    } else {
        sem->count++;
    }

    irq_leave_protection(irq_state);
}

int sys_sem_count (sem_t * sem) {
    irq_state_t  irq_state = irq_enter_protection();
    int count = sem->count;
    irq_leave_protection(irq_state);
    return count;
}