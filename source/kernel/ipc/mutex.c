#include "mutex.h"
#include "cpu.h"
void sys_mutex_init (mutex_t * mutex) {
    mutex->locked_count = 0;
    mutex->owner = (task_t *)0;
    list_init(&mutex->wait_list);
}

void sys_mutex_lock (mutex_t * mutex) {
    irq_state_t  irq_state = irq_enter_protection();

    task_t * curr = get_cur_task();
    if (mutex->locked_count == 0) {
        // 没有任务占用，占用之
        mutex->locked_count = 1;
        mutex->owner = curr;
    } else if (mutex->owner == curr) {
        // 已经为当前任务所有，只增加计数
        mutex->locked_count++;
    } else {
        // 有其它任务占用，则进入队列等待
        set_cur_task(NULL);
        curr->state = TASK_WAITING;
        list_insert_last(&mutex->wait_list, &curr->node);
        schedul();
    }

    irq_leave_protection(irq_state);
}

void sys_mutex_unlock (mutex_t * mutex) {
    irq_state_t  irq_state = irq_enter_protection();

    // 只有锁的拥有者才能释放锁
    task_t * curr = get_cur_task();
    if (mutex->owner == curr) {
        if (--mutex->locked_count == 0) {
            // 减到0，释放锁
            mutex->owner = NULL;

            // 如果队列中有任务等待，则立即唤醒并占用锁
            if (list_count(&mutex->wait_list)) {
                list_node_t * task_node = list_remove_first(&mutex->wait_list);
                task_t * task = list_node_parent(task_node, task_t, node);
                set_task_to_ready_list(task);

                // 在这里占用，而不是在任务醒后占用，因为可能抢不到
                mutex->locked_count = 1;
                mutex->owner = task;
                //在这里立马调度，怕别的进程抢不到
                schedul();
            }
        }
    }

    irq_leave_protection(irq_state);
}